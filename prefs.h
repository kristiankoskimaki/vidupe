#ifndef PREFS_H
#define PREFS_H

#include <QWidget>
#include "thumbnail.h"

class Prefs
{
public:
    enum _modes { _PHASH, _SSIM };

    QWidget *_mainwPtr = nullptr;               //pointer to MainWindow, for connecting signals to it's slots

    int _comparisonMode = _PHASH;
    int _thumbnails = thumb4;
    int _numberOfVideos = 0;
    int _ssimBlockSize = 16;

    double _thresholdSSIM = 0.89;
    int _thresholdPhash = 57;

    int _differentDurationModifier = 4;
    int _sameDurationModifier = 1;
};

#endif // PREFS_H
