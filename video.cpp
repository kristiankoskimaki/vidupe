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
        emit sendStatusMessage(QStringLiteral("ERROR: Out of memory"));
    else if(ret == _failure)
        emit rejectVideo(this);
    else if(hash == 0)                 //all screen captures black
        emit rejectVideo(this);
    else
        emit acceptVideo(QStringLiteral("[%1] %2").arg(QTime::currentTime().toString(), QDir::toNativeSeparators(filename)));
}

void Video::getMetadata(const QString &filename)
{
    QProcess probe;
    probe.setProcessChannelMode(QProcess::MergedChannels);
    probe.start(QStringLiteral("ffmpeg -hide_banner -i \"%1\"").arg(QDir::toNativeSeparators(filename)));
    probe.waitForFinished();

    bool rotatedOnce = false;
    const QString analysis(probe.readAllStandardOutput());
    const QStringList analysisLines = analysis.split(QStringLiteral("\r\n"));
    for(int i=0; i<analysisLines.length(); i++)
    {
        QString line = analysisLines.value(i);
        if(line.contains(QStringLiteral(" Duration:")))
        {
            const QString time = line.split(QStringLiteral(" ")).value(3);
            if(time == QLatin1String("N/A,"))
                duration = 0;
            else
            {
                const int h  = time.midRef(0,2).toInt();
                const int m  = time.midRef(3,2).toInt();
                const int s  = time.midRef(6,2).toInt();
                const int ms = time.midRef(9,2).toInt();
                duration = h*60*60*1000 + m*60*1000 + s*1000 + ms*10;
            }
            bitrate = line.split(QStringLiteral("bitrate: ")).value(1).split(QStringLiteral(" ")).value(0).toUInt();
        }
        if(line.contains(QStringLiteral(" Video:")) &&
          (line.contains(QStringLiteral("kb/s")) || line.contains(QStringLiteral(" fps"))))
        {
            line = line.replace(QRegExp(QStringLiteral("\\([^\\)]+\\)")), QStringLiteral(""));
            codec = line.split(QStringLiteral(" ")).value(7).replace(QStringLiteral(","), QStringLiteral(""));
            const QString resolution = line.split(QStringLiteral(",")).value(2);
            width = static_cast<ushort>(resolution.split(QStringLiteral("x")).value(0).toInt());
            height = static_cast<ushort>(resolution.split(QStringLiteral("x")).value(1)
                                                   .split(QStringLiteral(" ")).value(0).toInt());
            const QStringList fields = line.split(QStringLiteral(","));
            for(ushort j=0; j<fields.length(); j++)
                if(fields.at(j).contains(QStringLiteral("fps")))
                {
                    framerate = QStringLiteral("%1").arg(fields.at(j)).remove(QStringLiteral("fps")).toDouble();
                    framerate = round(framerate * 10) / 10;     //round to one decimal point
                }
        }
        if(line.contains(QStringLiteral(" Audio:")))
        {
            const QString audioCodec = line.split(QStringLiteral(" ")).value(7);
            const QString rate = line.split(QStringLiteral(",")).value(1);
            QString channels = line.split(QStringLiteral(",")).value(2);
            if(channels == QLatin1String(" 1 channels"))
                channels = QStringLiteral(" mono");
            else if(channels == QLatin1String(" 2 channels"))
                channels = QStringLiteral(" stereo");
            audio = QStringLiteral("%1%2%3").arg(audioCodec, rate, channels);
            const QString kbps = line.split(QStringLiteral(",")).value(4).split(QStringLiteral("kb/s")).value(0);
            if(!kbps.isEmpty() && kbps != QStringLiteral(" 0 "))
                audio = QStringLiteral("%1%2kb/s").arg(audio, kbps);
        }
        if(line.contains(QStringLiteral("rotate")) && !rotatedOnce)
        {
            const int rotate = line.split(QStringLiteral(":")).value(1).toInt();
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
            frame.load(&captureBuffer, "JPG");              //was saved in cache as small size, resize to original
            frame = frame.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
    image = minimizeImage(image);
    QBuffer buffer(&thumbnail);
    image.save(&buffer, "JPG", _jpegQuality);    //save GUI thumbnail as tiny JPEG to conserve memory

    cv::Mat mat = cv::Mat(image.height(), image.width(), CV_8UC3, image.bits(), static_cast<uint>(image.bytesPerLine()));
    resize(mat, mat, cv::Size(_ssimSize, _ssimSize), 0, 0, cv::INTER_AREA);
    cvtColor(mat, grayThumb, cv::COLOR_BGR2GRAY);
    grayThumb.convertTo(grayThumb, CV_32F);
    hash = computePhash(mat);
}

uint64_t Video::computePhash(const cv::Mat &input)
{
    cv::Mat resizeImg, grayImg, grayFImg, dctImg, topLeftDCT;

    cv::resize(input, resizeImg, cv::Size(_pHashSize, _pHashSize), 0, 0, cv::INTER_LINEAR_EXACT);
    cv::cvtColor(resizeImg, grayImg, cv::COLOR_BGR2GRAY);           //resize image to 32x32 grayscale

    int shadesOfGray = 0;
    uchar* pixel = reinterpret_cast<uchar*>(grayImg.data);          //pointer to pixel values, starts at first one
    const uchar* lastPixel = pixel + _pHashSize * _pHashSize;
    const uchar firstPixel = *pixel;

    for(pixel++; pixel<lastPixel; pixel++)              //skip first element since that one is already firstPixel
        shadesOfGray += qAbs(firstPixel - *pixel);      //compare all pixels with first one, tabulate differences
    if(shadesOfGray < _almostBlackBitmap)
        return 0;                                       //reject video if capture was (almost) monochrome

    grayImg.convertTo(grayFImg, CV_32F);
    cv::dct(grayFImg, dctImg);                          //compute DCT (discrete cosine transform)
    dctImg(cv::Rect(0, 0, 8, 8)).copyTo(topLeftDCT);    //use only upper left 8*8 transforms (most significant ones)

    const float firstElement = *reinterpret_cast<float*>(topLeftDCT.data);      //compute avg but skip first element
    const float average = (static_cast<float>(cv::sum(topLeftDCT)[0]) - firstElement) / 63;         //(it's very big)

    uint64_t hash = 0;
    float* transform = reinterpret_cast<float*>(topLeftDCT.data);
    const float* endOfData = transform + 64;
    for(int i=0; transform<endOfData; i++, transform++)             //construct hash from all 8x8 bits
        if(*transform > average)
            hash |= 1ULL << i;                                      //larger than avg = 1, smaller than avg = 0

    return hash;
}

QImage Video::minimizeImage(const QImage &image) const
{
    if(image.width() > image.height())
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

    QString paddedHours = QStringLiteral("%1").arg(hours);
    if(hours < 10)
        paddedHours = QStringLiteral("0%1").arg(paddedHours);

    QString paddedMinutes = QStringLiteral("%1").arg(minutes);
    if(minutes < 10)
        paddedMinutes = QStringLiteral("0%1").arg(paddedMinutes);

    QString paddedSeconds = QStringLiteral("%1").arg(seconds);
    if(seconds < 10)
        paddedSeconds = QStringLiteral("0%1").arg(paddedSeconds);

    return QStringLiteral("%1:%2:%3.%4").arg(paddedHours, paddedMinutes, paddedSeconds).arg(msecs);
}

QImage Video::captureAt(const short &percent, const short &ofDuration)
{
    const QTemporaryDir tempDir;
    if(!tempDir.isValid())
        return QImage();

    const QString screenshot = QStringLiteral("%1/vidupe%2.bmp").arg(tempDir.path()).arg(percent);
    QProcess ffmpeg;
    const QString ffmpegCommand = QStringLiteral("ffmpeg -ss %1 -i \"%2\" -an -frames:v 1 -pix_fmt rgb24 %3")
                                  .arg(msToHHMMSS(duration * (percent * ofDuration) / (100 * 100)),
                                  QDir::toNativeSeparators(filename), QDir::toNativeSeparators(screenshot));
    ffmpeg.start(ffmpegCommand);
    ffmpeg.waitForFinished(10000);

    QImage img(screenshot, "BMP");
    QFile::remove(screenshot);
    return img;
}
