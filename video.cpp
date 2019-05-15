#include "video.h"
#include "thumbnail.h"
#include "db.h"
#include <QTextEdit>
#include <QProcess>
#include <QBuffer>

ushort _jpegQuality = _okJpegQuality;

Video::Video(QWidget &parent, const QString &userFilename, const int &numberOfVideos, const short &userThumbnails)
{
    filename = userFilename;
    thumbnailMode = userThumbnails;

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
    if(!cache.readMetadata(*this))      //check first if video properties are cached
    {
        getMetadata(filename);          //if not, read them with ffmpeg
        cache.writeMetadata(*this);
    }

    if(width == 0 || height == 0 || duration == 0)
    {
        emit rejectVideo(this);
        return;
    }

    const ushort ret = takeScreenCaptures(cache);
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
            if(kbps != "" && kbps != " 0 ")
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

ushort Video::takeScreenCaptures(const Db &cache)
{
    Thumbnail thumb(thumbnailMode);

    const ushort mergedWidth = thumb.cols() * width, mergedHeight = thumb.rows() * height;
    const int mergedScreenSize = mergedWidth * mergedHeight * _BPP;
    uchar *mergedScreenCapture;
    try {
        mergedScreenCapture = new uchar[mergedScreenSize];
    }
    catch (std::bad_alloc &ex) {
        Q_UNUSED (ex);
        return _outOfMemory;
    }

    double ofDuration = 1.0;
    const QVector<short> percentages = thumb.percentages();
    int thumbnail = percentages.count();

    while(--thumbnail >= 0)         //screen captures are taken in reverse order so errors are found early
    {
        QImage img;
        QByteArray cachedImage = cache.readCapture(percentages[thumbnail]);
        QBuffer captureBuffer(&cachedImage);
        bool writeToCache = false;

        if(!cachedImage.isNull())   //image was already in cache
        {
            img.load(&captureBuffer, "JPG");
            img = img.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                     .convertToFormat(QImage::Format_RGB888);
        }
        else
        {
            img = captureAt(percentages[thumbnail], ofDuration);
            if(img.isNull())        //taking a screen capture may fail if video is broken
            {
                if(ofDuration >= _videoStillUsable)     //retry a few times, always starting closer to beginning
                {
                    ofDuration = ofDuration - _goBackwardsPercent;
                    thumbnail = percentages.count();
                    continue;
                }
                delete[] mergedScreenCapture;           //video is mostly garbage or seeking impossible, skip it
                return _failure;
            }
            writeToCache = true;
        }

        const int mergedOrigin = thumb.origin(this, percentages[thumbnail]);
        if(mergedOrigin + (img.height()-1) * mergedWidth * _BPP + img.width() * _BPP > mergedScreenSize)
        {   //prevent crash if screen capture has larger dimensions than buffer size
            emit sendStatusMessage(QString("ERROR: Buffer overflow prevented in %1 (%2x%3, expected %4x%5)").
                                   arg(QDir::toNativeSeparators(filename)).
                                   arg(img.width()).arg(img.height()).arg(width).arg(height));
            delete[] mergedScreenCapture;
            return _failure;
        }

        const size_t bytesPerLine = static_cast<size_t>(img.width() * _BPP);
        for(int line=0; line<img.height(); line++)  //write screen capture line by line into thumbnail
        {
            uchar *writeLineTo = mergedScreenCapture + mergedOrigin + line * mergedWidth * _BPP;
            memcpy(writeLineTo, img.constScanLine(line), bytesPerLine);
        }

        if(writeToCache)
        {
            img = minimizeImage(img);                                   //shrink image = smaller cache
            img.save(&captureBuffer, "JPG", _okJpegQuality);            //blurry jpg may actually increase
            cache.writeCapture(percentages[thumbnail], cachedImage);    //comparison accuracy (less strict)
        }
    }

    processThumbnail(mergedScreenCapture, mergedWidth, mergedHeight);

    delete[] mergedScreenCapture;
    return _success;
}

void Video::processThumbnail(const uchar *mergedScreenCapture, const ushort &mergedWidth, const ushort &mergedHeight)
{
    //the thumbnail is saved in the video object so it can be instantly loaded in GUI,
    //but resized small to save memory (there may be thousands of files)
    QImage smallImage = QImage(mergedScreenCapture, mergedWidth, mergedHeight, mergedWidth*_BPP, QImage::Format_RGB888);
    smallImage = minimizeImage(smallImage);
    QBuffer buffer(&thumbnail);
    smallImage.save(&buffer, "JPG", _jpegQuality);
    thumbnail = qCompress(thumbnail, _zlibCompression);

    hash = calculateHash(mergedScreenCapture, mergedWidth, mergedHeight);

    using namespace cv;
    QImage ssim = QImage(mergedScreenCapture, mergedWidth, mergedHeight, mergedWidth*_BPP, QImage::Format_RGB888);
    ssim = ssim.rgbSwapped();   //OpenCV uses BGR instead of RGB
    Mat mat = Mat(ssim.height(), ssim.width(), CV_8UC3, ssim.bits(), static_cast<uint>(ssim.bytesPerLine()));
    resize(mat, mat, Size(16, 16), 0, 0, INTER_AREA);   //16x16 seems to suffice, larger size has slower comparison
    cvtColor(mat, grayThumb, CV_BGR2GRAY);
    grayThumb.convertTo(grayThumb, CV_64F);
}

QImage Video::minimizeImage(const QImage &image) const
{
    if(image.width() >= image.height())
    {
        if(image.width() > _thumbnailMaxWidth)
            return image.scaledToWidth(_thumbnailMaxWidth, Qt::SmoothTransformation);
    }
    else if(image.height() > _thumbnailMaxHeight)
        return image.scaledToHeight(_thumbnailMaxHeight, Qt::SmoothTransformation);

    return image;
}

QImage Video::captureAt(const short &percent, const double &ofDuration)
{
    const QTemporaryDir tempDir;
    if(!tempDir.isValid())
        return QImage();

    const QString screenshot = QString("%1/vidupe%2.bmp").arg(tempDir.path()).arg(percent);

    QProcess ffmpeg;
    const QString ffmpegCommand = QString("ffmpeg -ss %1 -i \"%2\" -an -frames:v 1 -pix_fmt rgb24 %3")
                                  .arg(Thumbnail::msToHHMMSS(static_cast<qint64>( percent * ofDuration * duration / 100) ),
                                  QDir::toNativeSeparators(filename), QDir::toNativeSeparators(screenshot));
    ffmpeg.start(ffmpegCommand);
    ffmpeg.waitForFinished(10000);

    QImage img(screenshot, "BMP");
    img = img.convertToFormat(QImage::Format_RGB888);
    QFile::remove(screenshot);
    return img;
}
