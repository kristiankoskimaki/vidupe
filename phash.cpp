#include "video.h"

uint64_t Video::calculateHash(QImage &image) const
{
    image = image.scaled(_blockSize, _blockSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QVector<double> grayImage;
    grayImage.reserve(_blockSize * _blockSize);
    if(convertGrayscale(image, grayImage) == true)      //also reject video if happened to capture monochrome image
        return 0;

    QVector<double> temp(_blockSize * _blockSize);
    discreteCosineTransform(grayImage, _blockSize);

    double avgTransform = 0;
    QVector<double> reducedTransform;
    reducedTransform.reserve(_smallBlock * _smallBlock);

    for(ushort i=0; i<_smallBlock; i++)                 //only use upper left 8*8 transforms (most significant ones)
        for(ushort j=0; j<_smallBlock; j++)
        {
            const double element = grayImage[i * _blockSize + j];
            reducedTransform.push_back(element);
            avgTransform = avgTransform + element;
        }
                                                        //skip first term since it differs much from others
    avgTransform = (avgTransform - grayImage[0]) / (_smallBlock * _smallBlock - 1);

    uint64_t hash = 0;      //construct hash from all 64 bits: larger than avg = 1, smaller than avg = 0
    for(ushort i=0; i<_smallBlock*_smallBlock; i++)
        if(reducedTransform[i] > avgTransform)
            hash |= 1ULL << i;

    return hash;
}

bool Video::convertGrayscale(const QImage &image, QVector<double> &grayImage) const
{
    const QRgb firstPixel = image.pixel(0, 0);
    bool arePixelsIdentical = true;

    for(ushort i=0; i<_blockSize; i++)
        for(ushort j=0; j<_blockSize; j++)
        {
            const QRgb pixel = image.pixel(i, j);
            grayImage.push_back(qGray(qRed(pixel), qGreen(pixel), qBlue(pixel)));

            if(pixel != firstPixel)
                arePixelsIdentical = false;
        }

    return(arePixelsIdentical);
}

void Video::discreteCosineTransform(QVector<double> &vec, const int &length) const
{
    double iCoeff, jCoeff, dct, sum;

    for(ushort i=0; i<length; i++)
    {
        for(ushort j=0; j<length; j++)
        {
            if(i == 0)
                iCoeff = 1 / sqrt(length);
            else
                iCoeff = sqrt(2) / sqrt(length);
            if(j == 0)
                jCoeff = 1 / sqrt(length);
            else
                jCoeff = sqrt(2) / sqrt(length);

            sum = 0;
            for(ushort k=0; k<length; k++)
            {
                for(ushort l=0; l<length; l++)
                {
                    dct = vec[k * length + l] *
                           cos((2 * k + 1) * i * M_PI / (2 * length)) *
                           cos((2 * l + 1) * j * M_PI / (2 * length));
                    sum = sum + dct;
                }
            }
            vec[i*length+j] = iCoeff * jCoeff * sum;
        }
    }
}
