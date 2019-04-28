#include "video.h"
#include "db.h"
#include <QCryptographicHash>
#include <QTextEdit>
#include <QProcess>
#include <QBuffer>

ushort _jpegQuality = _okJpegQuality;

Video::Video(QWidget &parent, const QString &userFilename, const int &numberOfVideos, const short &userThumbnails)
{
    filename = userFilename;
    thumbnails = userThumbnails;

    if(numberOfVideos > _hugeAmountVideos)       //save memory to avoid crash due to 32 bit limit
        _jpegQuality = _lowJpegQuality;

    QObject::connect(this, SIGNAL(rejectVideo(Video*)), &parent, SLOT(removeVideo(Video*)));
    QObject::connect(this, SIGNAL(acceptVideo(QString)), &parent, SLOT(addVideo(QString)));
    QObject::connect(this, SIGNAL(sendStatusMessage(QString)), &parent, SLOT(addStatusMessage(QString)));
}

void Video::run()
{
    if(!QFileInfo::exists(filename))
    {
        emit rejectVideo(this);
        return;
    }

    Db cache(filename);
    if(!cache.read(*this))              //check first if video properties are cached
    {
        getMetadata(filename);          //if not, read them with ffmpeg
        cache.write(*this);
    }

    if(width == 0 || height == 0 || duration == 0)
    {
        emit rejectVideo(this);
        return;
    }

    const ushort ret = takeScreenCaptures();
    if(ret == _outOfMemory)
        emit sendStatusMessage("ERROR: Out of memory");
    else if(ret == _failure)
        emit rejectVideo(this);
    else if(hash == 0)                 //all screen captures black
        emit rejectVideo(this);
    else
        emit acceptVideo(QString("[%1] %2").arg(QTime::currentTime().toString(), QDir::toNativeSeparators(filename)));
}

void Video::getMetadata(const QString &filename)
{
    QProcess probe;
    probe.setProcessChannelMode(QProcess::MergedChannels);
    probe.start(QString("ffmpeg -hide_banner -i \"%1\"").arg(QDir::toNativeSeparators(filename)));
    probe.waitForFinished();

    bool rotatedOnce = false;
    const QString analysis(probe.readAllStandardOutput());
    const QStringList analysisLines = analysis.split("\r\n");
    for(int i=0; i<analysisLines.length(); i++)
    {
        QString line = analysisLines.value(i);
        if(line.contains(" Duration:"))
        {
            const QString time = line.split(" ").value(3);
            if(time == "N/A,")
                duration = 0;
            else
            {
                const int h  = time.midRef(0,2).toInt();
                const int m  = time.midRef(3,2).toInt();
                const int s  = time.midRef(6,2).toInt();
                const int ms = time.midRef(9,2).toInt();
                duration = h*60*60*1000 + m*60*1000 + s*1000 + ms*10;
            }
            bitrate = line.split("bitrate: ").value(1).split(" ").value(0).toUInt();
        }
        if(line.contains(" Video:") && (line.contains("kb/s") || line.contains(" fps")))
        {
            line = line.replace(QRegExp("\\([^\\)]+\\)"), "");
            codec = line.split(" ").value(7).replace(",", "");
            const QString resolution = line.split(",").value(2);
            width = static_cast<ushort>(resolution.split("x").value(0).toInt());
            height = static_cast<ushort>(resolution.split("x").value(1).split(" ").value(0).toInt());
            const QStringList fields = line.split(",");
            for(ushort j=0; j<fields.length(); j++)
                if(fields.at(j).contains("fps"))
                {
                    framerate = QString("%1").arg(fields.at(j)).remove("fps").toDouble();
                    framerate = round(framerate * 10) / 10;     //round to one decimal point
                }
        }
        if(line.contains(" Audio:"))
        {
            const QString audioCodec = line.split(" ").value(7);
            const QString rate = line.split(",").value(1);
            QString channels = line.split(",").value(2);
            if(channels == " 1 channels")
                channels = " mono";
            else if(channels == " 2 channels")
                channels = " stereo";
            audio = QString("%1%2%3").arg(audioCodec, rate, channels);
            const QString kbps = line.split(",").value(4).split("kb/s").value(0);
            if(kbps != "")
                audio = QString("%1%2kb/s").arg(audio, kbps);
        }
        if(line.contains("rotate") && !rotatedOnce)
        {
            const int rotate = line.split(":").value(1).toInt();
            if(rotate == 90 || rotate == 270)
            {
                const ushort temp = width;
                width = height;
                height = temp;
            }
            rotatedOnce = true;     //rotate only once (AUDIO metadata can contain rotate keyword)
        }
    }

    const QFileInfo videoFile(filename);
    size = videoFile.size();
    modified = videoFile.lastModified();
}

QString Video::reencodeVideo(const QTemporaryDir &tempDir, int &reencodeStatus)
{
    const QString filenameReencoded = QString("%1/reencoded%2").arg(tempDir.path(),
                  filename.split(filename.left(filename.lastIndexOf("."))).value(1));

    QProcess encode;
    encode.setProcessChannelMode(QProcess::MergedChannels);
    encode.start(QString("ffmpeg -i \"%1\" -acodec copy -vcodec copy -map_metadata -1 \"%2\"").
                 arg(QDir::toNativeSeparators(filename), QDir::toNativeSeparators(filenameReencoded)));
    encode.waitForFinished();

    if(!QFileInfo::exists(filenameReencoded))
    {
        reencodeStatus = _reencodingFailed;
        return filenameReencoded;
    }
    const QFileInfo newFile(filenameReencoded);
    const QFileInfo oldFile(filename);
    if(static_cast<double>(newFile.size()) / static_cast<double>(oldFile.size()) > _smallestReencoded)
        getMetadata(filenameReencoded);          //hopefully metadata is correct now

    reencodeStatus = _reencoded;
    return filenameReencoded;
}

ushort Video::takeScreenCaptures()
{
    const int mergedScreenSize = width * height * _BPP * thumbnails;
    uchar *mergedScreenCapture;
    try {
        mergedScreenCapture = new uchar[mergedScreenSize];
    }
    catch (std::bad_alloc &ex) {
        Q_UNUSED (ex);
        return _outOfMemory;
    }
    ushort mergedWidth, mergedHeight;
    setMergedWidthAndHeight(mergedWidth, mergedHeight);

    const QTemporaryDir tempDir;
    if(!tempDir.isValid())
        return _failure;

    int reencodeStatus = _notReencoded;
    QString filenameReencoded;
    double ofDuration = 1.0;
    const short skipPercent = getSkipPercent();
    //screen captures are taken starting at the end (any errors are likely there)
    for(short percent=100-skipPercent; percent>0; percent-=skipPercent)
    {
        const QString videoFilename = reencodeStatus == _reencoded? filenameReencoded : filename;

        const QImage img = captureAt(videoFilename, tempDir, percent, ofDuration);
        //taking a screen capture may fail if video is broken (botched encoding, failed download)
        if(img.isNull())
        {
            if(reencodeStatus == _notReencoded && size < _reencodeMaxSize)      //try re-encoding first
            {
                filenameReencoded = reencodeVideo(tempDir, reencodeStatus);
                percent = 100;
                continue;
            }
            if(ofDuration >= _videoStillUsable)     //retry a few times, always starting closer to beginning
            {
                ofDuration = ofDuration - _goBackwardsPercent;
                percent = 100;
                continue;
            }

            delete[] mergedScreenCapture;       //video is mostly garbage or seeking impossible, skip it
            return _failure;
        }

        const int mergedOrigin = calculateOrigin(percent);
        if(mergedOrigin + (img.height()-1) * mergedWidth * _BPP + img.width() * _BPP > mergedScreenSize)
        {   //prevent crash if screen capture has larger dimensions than buffer size
            emit sendStatusMessage(QString("ERROR: Buffer overflow prevented in %1 (%2x%3, expected %4x%5)").
                                   arg(QDir::toNativeSeparators(filename)).
                                   arg(img.width()).arg(img.height()).arg(width).arg(height));
            delete[] mergedScreenCapture;
            return _failure;
        }

        for(int line=0; line<img.height(); line++)  //write screen capture line by line into thumbnail
        {
            const int writeLineTo = mergedOrigin + line * mergedWidth * _BPP;
            memcpy(mergedScreenCapture + writeLineTo, img.constScanLine(line), static_cast<size_t>(img.width()) * _BPP);
        }
    }

    processThumbnail(mergedScreenCapture, mergedWidth, mergedHeight);

    if(reencodeStatus != _notReencoded)
        needsReencoding = true;
    delete[] mergedScreenCapture;
    return _success;
}

void Video::processThumbnail(const uchar *mergedScreenCapture, const ushort &mergedWidth, const ushort &mergedHeight)
{
    //the thumbnail is saved in the video object so it can be instantly loaded in GUI,
    //but resized small to save memory (there may be thousands of files)
    QImage smallImage = QImage(mergedScreenCapture, mergedWidth, mergedHeight, mergedWidth*_BPP, QImage::Format_RGB888);

    if(mergedWidth >= mergedHeight)
    {
        if(mergedWidth > _thumbnailMaxWidth)
            smallImage = smallImage.scaledToWidth(_thumbnailMaxWidth, Qt::SmoothTransformation);
    }
    else if(mergedHeight > _thumbnailMaxHeight)
        smallImage = smallImage.scaledToHeight(_thumbnailMaxHeight, Qt::SmoothTransformation);

    QBuffer buffer(&thumbnail);
    smallImage.save(&buffer, "JPG", _jpegQuality);
    thumbnail = qCompress(thumbnail, _zlibCompression);

    calculateHash(mergedScreenCapture, mergedWidth, mergedHeight);

    using namespace cv;
    QImage ssim = QImage(mergedScreenCapture, mergedWidth, mergedHeight, mergedWidth*_BPP, QImage::Format_RGB888);
    ssim = ssim.rgbSwapped();   //OpenCV uses BGR instead of RGB
    Mat mat = Mat( ssim.height(), ssim.width(), CV_8UC3, const_cast<uchar *>(ssim.bits()),
                   static_cast<size_t>(ssim.bytesPerLine()) ).clone();
    resize(mat, mat, Size(16, 16), 0, 0, INTER_AREA);   //16x16 seems to suffice, larger size has slower comparison
    cvtColor(mat, grayThumb, CV_BGR2GRAY);
    grayThumb.convertTo(grayThumb, CV_64F);
}

QImage Video::captureAt(const QString &filename, const QTemporaryDir &tempDir, const short &percent, const double &ofDuration)
{
    int reencodeStatus;
    const QString videoFilename = needsReencoding? reencodeVideo(tempDir, reencodeStatus) : filename;
    const QString screenshot = QString("%1/vidupe%2.bmp").arg(tempDir.path()).arg(percent);

    QProcess ffmpeg;
    ffmpeg.setProcessChannelMode(QProcess::MergedChannels);
    const QString ffmpegCommand = QString("ffmpeg -ss %1 -i \"%2\" -an -frames:v 1 -pix_fmt rgb24 %3")
                                  .arg(msToHHMSS(static_cast<qint64>( percent * ofDuration * duration / 100) ),
                                  QDir::toNativeSeparators(videoFilename), QDir::toNativeSeparators(screenshot));
    ffmpeg.start(ffmpegCommand);
    ffmpeg.waitForFinished();

    QImage img(screenshot, "BMP");
    img = img.convertToFormat(QImage::Format_RGB888);
    QFile::remove(screenshot);
    return img;
}
