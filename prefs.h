#ifndef PREFS_H
#define PREFS_H

#include "thumbnail.h"

class Prefs
{
public:
    enum _modes { _PHASH, _SSIM };
    int _ComparisonMode = _PHASH;
    int _thumbnails = thumb4;

    int _thresholdPhash = 7;
    int _thresholdPhashOriginal = 7;

    double _thresholdSSIM = 0.90;
    int _ssimBlockSize = 16;

    int _differentDurationModifier = 4;
    int _sameDurationModifier = 1;
};

#endif // PREFS_H
