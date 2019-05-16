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

static constexpr ushort _BPP = 3;               //bits per pixel

class Video : public QObject, public QRunnable
{
    Q_OBJECT

public:
    Video(QWidget &parent, const QString &userFilename, const int &numberOfVideos, const ushort &userThumbnails=2*2);
    void run() override;

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
    QImage captureAt(const short &percent, const short &ofDuration=100);

signals:
    void acceptVideo(const QString) const;
    void rejectVideo(Video *) const;
    void sendStatusMessage(const QString) const;

private:
    static ushort _thumbnailMode;
    static ushort _jpegQuality;

    static constexpr ushort _success     = 0;
    static constexpr ushort _failure     = 1;
    static constexpr ushort _outOfMemory = 2;

    static constexpr ushort _okJpegQuality    = 60;
    static constexpr ushort _lowJpegQuality   = 25;
    static constexpr int    _hugeAmountVideos = 200000;

    static constexpr ushort _goBackwardsPercent = 7;    //if capture fails, retry but omit this much from end
    static constexpr ushort _videoStillUsable   = 86;   //86% of video duration is considered usable
    static constexpr int    _thumbnailMaxWidth  = 448;  //small size to save memory and cache space
    static constexpr int    _thumbnailMaxHeight = 336;
    static constexpr ushort _blockSize          = 32;   //phash generated from 32x32 image
};

#endif // VIDEO_H
