#include "video.h"

void Video::calculateHash(uchar *mergedScreenCapture, ushort &mergedWidth, ushort &mergedHeight)
{
    double transform[_blockSize * _blockSize];
    const ushort smallBlock = 8;

    QImage image = QImage(mergedScreenCapture, mergedWidth, mergedHeight, mergedWidth*_BPP, QImage::Format_RGB888);
    image = image.scaled(_blockSize, _blockSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    if(convertGrayscale(image) == true)                 //also reject video if happened to capture monochrome image
        return;
    discreteCosineTransform(image, transform);

    double reducedTransform[smallBlock*smallBlock];      //only use upper left 8*8 transforms (most significant ones)
    for(ushort i=0; i<smallBlock; i++)
        for(ushort j=0; j<smallBlock; j++)
            reducedTransform[i * smallBlock + j] = transform[i * _blockSize + j];

    double avgTransform = 0;
    for(ushort i=1; i<smallBlock*smallBlock; i++)       //skip first term since it differs much from others
        avgTransform += reducedTransform[i];
    avgTransform = avgTransform / (smallBlock * smallBlock - 1);

    //construct hash from all 64 bits: larger than avg = 1, smaller than avg = 0
    for(ushort i=0; i<smallBlock*smallBlock; i++)
        if(reducedTransform[i] > avgTransform)
            hash |= 1ULL << i;
}

bool Video::convertGrayscale(QImage &image) const
{
    const QRgb firstPixel = image.pixel(0, 0);
    bool arePixelsIdentical = true;

    for(ushort i=0; i<_blockSize; i++)
        for(ushort j=0; j<_blockSize; j++)
        {
            const QRgb pixel = image.pixel(i, j);
            const int gray = qGray(qRed(pixel), qGreen(pixel), qBlue(pixel));
            image.setPixel(i, j, static_cast<uchar>(gray));

            if(pixel != firstPixel)
                arePixelsIdentical = false;
        }

    return(arePixelsIdentical);
}

void Video::discreteCosineTransform(QImage &image, double *transform) const
{
    double iCoeff, jCoeff, dct, sum;

    for(ushort i=0; i<_blockSize; i++)
    {
        for(ushort j=0; j<_blockSize; j++)
        {
            if(i == 0)
                iCoeff = 1 / sqrt(_blockSize);
            else
                iCoeff = sqrt(2) / sqrt(_blockSize);
            if(j == 0)
                jCoeff = 1 / sqrt(_blockSize);
            else
                jCoeff = sqrt(2) / sqrt(_blockSize);

            sum = 0;
            for(ushort k=0; k<_blockSize; k++)
            {
                for(ushort l=0; l<_blockSize; l++)
                {
                    dct = image.pixel(k, l) *
                           cos((2 * k + 1) * i * M_PI / (2 * _blockSize)) *
                           cos((2 * l + 1) * j * M_PI / (2 * _blockSize));
                    sum = sum + dct;
                }
            }
            transform[_blockSize * i + j] = iCoeff * jCoeff * sum;
        }
    }
}
