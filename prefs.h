#ifndef PREFS_H
#define PREFS_H

class Prefs
{
public:
    enum _modes { _PHASH, _SSIM };
    short _ComparisonMode = _PHASH;
    short _thumbnails = 2*2;

    short _thresholdPhash = 7;
    short _thresholdPhashOriginal = 7;

    double _thresholdSSIM = 0.90;
    short _ssimBlockSize = 16;

    short _differentDurationModifier = 0;
    short _sameDurationModifier = 0;
};

#endif // PREFS_H
