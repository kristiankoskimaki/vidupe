#include "video.h"
#include "thumbnail.h"
#include "db.h"
#include <QTextEdit>
#include <QProcess>
#include <QPainter>
#include <QBuffer>

ushort Video::_thumbnailMode = thumb4;
ushort Video::_jpegQuality = _okJpegQuality;

Video::Video(QWidget &parent, const QString &userFilename, const int &numberOfVideos, const ushort &userThumbnails)
{
    filename = userFilename;
    _thumbnailMode = userThumbnails;

    if(numberOfVideos > _hugeAmountVideos)       //save memory to avoid crash due to 32 bit limit
        _jpegQuality = _lowJpegQuality;

    QObject::connect(this, SIGNAL(rejectVideo(Video *)), &parent, SLOT(removeVideo(Video *)));
    QObject::connect(this, SIGNAL(acceptVideo(const QString &)), &parent, SLOT(addVideo(const QString &)));
    QObject::connect(this, SIGNAL(sendStatusMessage(const QString &)), &parent, SLOT(addStatusMessage(const QString &)));
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
    Thumbnail thumb(_thumbnailMode);
    QImage thumbnail(thumb.cols() * width, thumb.rows() * height, QImage::Format_RGB888);
    const QVector<short> percentages = thumb.percentages();
    int capture = percentages.count();
    short ofDuration = 100;

    while(--capture >= 0)           //screen captures are taken in reverse order so errors are found early
    {
        QImage frame;
        QByteArray cachedImage = cache.readCapture(percentages[capture]);
        QBuffer captureBuffer(&cachedImage);
        bool writeToCache = false;

        if(!cachedImage.isNull())   //image was already in cache
        {
            frame.load(&captureBuffer, "JPG");
            frame = frame.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                         .convertToFormat(QImage::Format_RGB888);
        }
        else
        {
            frame = captureAt(percentages[capture], ofDuration);
            if(frame.isNull())                                  //taking screen capture may fail if video is broken
            {
                if(ofDuration >= _videoStillUsable)             //retry a few times, always closer to beginning
                {
                    ofDuration = ofDuration - _goBackwardsPercent;
                    capture = percentages.count();
                    continue;
                }
                return _failure;
            }
            writeToCache = true;
        }
        if(frame.width() > width || frame.height() > height)    //metadata parsing error or variable resolution
            return _failure;

        QPainter painter(&thumbnail);                           //copy captured frame into right place in thumbnail
        painter.drawImage(capture % thumb.cols() * width, capture / thumb.cols() * height, frame);

        if(writeToCache)
        {
            frame = minimizeImage(frame);                               //shrink image = smaller cache
            frame.save(&captureBuffer, "JPG", _okJpegQuality);          //blurry jpg may actually increase
            cache.writeCapture(percentages[capture], cachedImage);      //comparison accuracy (less strict)
        }
    }
    processThumbnail(thumbnail);
    return _success;
}

void Video::processThumbnail(QImage &image)
{
    using namespace cv;     //note: mat is now RGB-swapped since opencv uses BGR (moot, since we use grayscale)
    Mat mat = Mat(image.height(), image.width(), CV_8UC3, image.bits(), static_cast<uint>(image.bytesPerLine()));
    resize(mat, mat, Size(16, 16), 0, 0, INTER_AREA);   //16x16 seems to suffice, larger size has slower comparison
    cvtColor(mat, grayThumb, CV_BGR2GRAY);
    grayThumb.convertTo(grayThumb, CV_64F);

    QBuffer buffer(&thumbnail);
    minimizeImage(image).save(&buffer, "JPG", _jpegQuality);    //save GUI thumbnail as tiny JPEG to conserve memory
    hash = calculateHash(image);
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

QString Video::msToHHMMSS(const qint64 &time) const
{
    const ushort hours   = time / (1000*60*60) % 24;
    const ushort minutes = time / (1000*60) % 60;
    const ushort seconds = time / 1000 % 60;
    const ushort msecs   = time % 1000;

    QString paddedHours = QString("%1").arg(hours);
    if(hours < 10)
        paddedHours = "0" + paddedHours;

    QString paddedMinutes = QString("%1").arg(minutes);
    if(minutes < 10)
        paddedMinutes = "0" + paddedMinutes;

    QString paddedSeconds = QString("%1").arg(seconds);
    if(seconds < 10)
        paddedSeconds = "0" + paddedSeconds;

    return QString("%1:%2:%3.%4").arg(paddedHours, paddedMinutes, paddedSeconds).arg(msecs);
}

QImage Video::captureAt(const short &percent, const short &ofDuration)
{
    const QTemporaryDir tempDir;
    if(!tempDir.isValid())
        return QImage();

    const QString screenshot = QString("%1/vidupe%2.bmp").arg(tempDir.path()).arg(percent);
    QProcess ffmpeg;
    const QString ffmpegCommand = QString("ffmpeg -ss %1 -i \"%2\" -an -frames:v 1 -pix_fmt rgb24 %3")
                                  .arg(msToHHMMSS(duration * (percent * ofDuration) / (100 * 100)),
                                  QDir::toNativeSeparators(filename), QDir::toNativeSeparators(screenshot));
    ffmpeg.start(ffmpegCommand);
    ffmpeg.waitForFinished(10000);

    QImage img(screenshot, "BMP");
    img = img.convertToFormat(QImage::Format_RGB888);
    QFile::remove(screenshot);
    return img;
}
