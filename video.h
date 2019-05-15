#ifndef VIDEO_H
#define VIDEO_H

#include <QDebug>
#include <QRunnable>
#include <QDateTime>
#include <QImage>
#include <QTemporaryDir>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include "thumbnail.h"
#include "db.h"

const ushort _BPP = 3;                          //bits per pixel
const ushort _blockSize = 32;                   //phash generated from 32x32 image

const ushort _success     = 0;
const ushort _failure     = 1;
const ushort _outOfMemory = 2;

const double _goBackwardsPercent = 0.07;        //if capture fails, retry but omit this much from ending...
const double _videoStillUsable   = 0.86;        //...but no more than 14% of total runtime
const int    _thumbnailMaxWidth  = 448;
const int    _thumbnailMaxHeight = 336;
const ushort _zlibCompression    = 9;

extern ushort _jpegQuality;
const ushort _okJpegQuality      = 60;
const ushort _lowJpegQuality     = 25;
const int    _hugeAmountVideos   = 200000;

class Video : public QObject, public QRunnable
{
    Q_OBJECT

public:
    Video(QWidget &parent, const QString &userFilename, const int &numberOfVideos, const short &userThumbnails=2*2);
    void run() override;

    short thumbnailMode = thumb4;

    QString filename = "";
    qint64 size = 0;
    QDateTime modified;
    qint64 duration = 0;
    uint bitrate = 0;
    double framerate = 0;
    QString codec = "";
    QString audio = "";
    uint64_t hash = 0;
    ushort width = 0;
    ushort height = 0;
    QByteArray thumbnail;
    cv::Mat grayThumb;

private slots:
    void getMetadata(const QString &filename);
    ushort takeScreenCaptures(const Db &cache);
    void processThumbnail(const uchar *mergedCapture, const ushort &mergedWidth, const ushort &mergedHeight);
    QImage minimizeImage(const QImage &image) const;

    uint64_t calculateHash(const uchar *imageData, const ushort &width, const ushort &height) const;
    bool convertGrayscale(QImage &image) const;
    void discreteCosineTransform(const QImage &image, double *transform) const;

public slots:
    QImage captureAt(const short &percent, const double &ofDuration=1.0);

signals:
    void acceptVideo(const QString) const;
    void rejectVideo(Video *) const;
    void sendStatusMessage(const QString) const;
};

#endif // VIDEO_H
