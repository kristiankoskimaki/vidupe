#ifndef PREFS_H
#define PREFS_H

#include "thumbnail.h"

class Prefs
{
public:
    enum _modes { _PHASH, _SSIM };
    short _ComparisonMode = _PHASH;
    ushort _thumbnails = thumb4;

    short _thresholdPhash = 7;
    short _thresholdPhashOriginal = 7;

    double _thresholdSSIM = 0.90;
    short _ssimBlockSize = 16;

    short _differentDurationModifier = 4;
    short _sameDurationModifier = 1;
};

#endif // PREFS_H
