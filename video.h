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

class Video : public QObject, public QRunnable
{
    Q_OBJECT

public:
    Video(const QWidget &parent, const QString &userFilename, const int &numberOfVideos, const ushort &userThumbnails);
    void run();

    QString filename;
    qint64 size = 0;
    QDateTime modified;
    qint64 duration = 0;
    uint bitrate = 0;
    double framerate = 0;
    QString codec;
    QString audio;
    ushort width = 0;
    ushort height = 0;
    QByteArray thumbnail;
    cv::Mat grayThumb;
    uint64_t hash = 0;

private slots:
    void getMetadata(const QString &filename);
    ushort takeScreenCaptures(const Db &cache);
    void processThumbnail(QImage &image);
    uint64_t computePhash(const cv::Mat &input);
    QImage minimizeImage(const QImage &image) const;
    QString msToHHMMSS(const qint64 &time) const;

public slots:
    QImage captureAt(const short &percent, const short &ofDuration=100) const;

signals:
    void acceptVideo(const QString &filename) const;
    void rejectVideo(Video *deleteMe) const;

private:
    static ushort _thumbnailMode;
    static ushort _jpegQuality;

    enum _returnValues { _success, _failure };

    static constexpr ushort _okJpegQuality    = 60;
    static constexpr ushort _lowJpegQuality   = 25;
    static constexpr int    _hugeAmountVideos = 200000;

    static constexpr ushort _goBackwardsPercent = 7;    //if capture fails, retry but omit this much from end
    static constexpr ushort _videoStillUsable   = 86;   //86% of video duration is considered usable
    static constexpr int    _thumbnailMaxWidth  = 448;  //small size to save memory and cache space
    static constexpr int    _thumbnailMaxHeight = 336;
    static constexpr ushort _pHashSize          = 32;   //phash generated from 32x32 image
    static constexpr ushort _ssimSize           = 16;   //larger than 16x16 seems to have slower comparison
    static constexpr ushort _almostBlackBitmap  = 1500; //monochrome thumbnail if less shades of gray than this
};

#endif // VIDEO_H
