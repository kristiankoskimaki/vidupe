#include "video.h"

uint64_t Video::calculateHash(QImage &image) const
{
    image = image.scaled(_blockSize, _blockSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QVector<double> grayImage;
    grayImage.reserve(_blockSize * _blockSize);
    if(convertGrayscale(image, grayImage) == true)      //also reject video if happened to capture monochrome image
        return 0;

    QVector<double> temp(_blockSize * _blockSize);
    fastDCTransform(grayImage.data(), temp.data(), _blockSize * _blockSize);   //perform discrete cosine transform (DCT)

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

/*
Fast discrete cosine transform algorithms (C++)

Copyright (c) 2017 Project Nayuki. (MIT License)
https://www.nayuki.io/page/fast-discrete-cosine-transform-algorithms

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:
- The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
- The Software is provided "as is", without warranty of any kind, express or
  implied, including but not limited to the warranties of merchantability,
  fitness for a particular purpose and noninfringement. In no event shall the
  authors or copyright holders be liable for any claim, damages or other
  liability, whether in an action of contract, tort or otherwise, arising from,
  out of or in connection with the Software or the use or other dealings in the
  Software.
*/
//Byeong Gi Lee: “FCT - A Fast Cosine Transform” IEEE 1984
void Video::fastDCTransform(double vec[], double temp[], size_t len) const
{
    if(len == 1)
        return;

    size_t halfLen = len / 2;
    for(size_t i=0; i<halfLen; i++)
    {
        double x = vec[i];
        double y = vec[len - 1 - i];
        temp[i] = x + y;
        temp[i + halfLen] = (x - y) / (cos((i + 0.5) * M_PI / len) * 2);
    }

    fastDCTransform(temp, vec, halfLen);
    fastDCTransform(&temp[halfLen], vec, halfLen);

    for(size_t i=0; i<halfLen-1; i++)
    {
        vec[i * 2 + 0] = temp[i];
        vec[i * 2 + 1] = temp[i + halfLen] + temp[i + halfLen + 1];
    }
    vec[len - 2] = temp[halfLen - 1];
    vec[len - 1] = temp[len - 1];
}
