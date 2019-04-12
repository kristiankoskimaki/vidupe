#ifndef VIDEO_H
#define VIDEO_H

#include <QRunnable>
#include <QDateTime>
#include <QImage>
#include <QTemporaryDir>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>

const ushort _BPP = 3;                          //bits per pixel
const ushort _blockSize = 32;                   //phash generated from 32x32 image

const ushort _success     = 0;
const ushort _failure     = 1;
const ushort _retry       = 2;
const ushort _outOfMemory = 3;

const uint   _reencodeMaxSize    = 50000000;    //broken videos (50 Mb or less) will be re-encoded
const double _smallestReencoded  = 0.10;        //re-encoded file cannot be <10% of original size
const double _goBackwardsPercent = 0.07;        //if capture fails, retry but omit this much from ending...
const double _videoStillUsable   = 0.70;        //...but no more than 30% of total runtime
const uint   _thumbnailMaxWidth  = 448;
const uint   _thumbnailMaxHeight = 336;
const ushort _zlibCompression    = 9;

extern ushort _jpegQuality;
const ushort _okJpegQuality      = 60;          //default
const ushort _lowJpegQuality     = 25;
const int    _hugeAmountVideos   = 200000;

const ushort _notReencoded       = 0;           //video re-encoding status
const ushort _reencoded          = 1;
const ushort _reencodingFailed   = 2;

class Video : public QObject, public QRunnable
{
    Q_OBJECT

public:
    Video(QWidget &parent, const QString &userFilename, const int &numberOfVideos, const short &userThumbnails=2*2);
    void run() override;

    QWidget *mainWindow = nullptr;
    int videoAmount = 0;
    short thumbnails = 2*2;

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
    bool needsReencoding = false;

private slots:
    void getMetadata(const QString &filename);
    QString reencodeVideo(const QTemporaryDir &tempDir, int &reencodeStatus);
    ushort takeScreenCaptures();

    void setMergedWidthAndHeight(ushort &mergedWidth, ushort &mergedHeight) const;
    ushort getSkipPercent() const;
    QString msToHHMSS(const qint64 &time) const;
    int calculateOrigin(const short &percent) const;

    void calculateHash(uchar *mergedScreenCapture, ushort &mergedWidth, ushort &mergedHeight);
    void convertGrayscale(QImage &image) const;
    void discreteCosineTransform(QImage &image, double *transform) const;

public slots:
    QImage captureAt(const QString &filename, const QTemporaryDir &tempDir, const short &percent, const double &ofDuration=1.0);

signals:
    void acceptVideo(const QString) const;
    void rejectVideo(Video *) const;
    void sendStatusMessage(const QString) const;
};

#endif // VIDEO_H
