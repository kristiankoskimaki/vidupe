#include "thumbnail.h"
#include "video.h"

QString Thumbnail::msToHHMMSS(const qint64 &time)
{
    const ushort hours   = time / (1000*60*60) % 24;
    const ushort minutes = time / (1000*60) % 60;
    const ushort seconds = time / 1000 % 60;
    const ushort milliseconds = time % 1000;

    QString paddedHours;
    if(hours < 10)
        paddedHours = QString("0%1").arg(hours);
    else
        paddedHours = QString("%1").arg(hours);

    QString paddedMinutes;
    if(minutes < 10)
        paddedMinutes = QString("0%1").arg(minutes);
    else
        paddedMinutes = QString("%1").arg(minutes);

    QString paddedSeconds;
    if(seconds < 10)
        paddedSeconds = QString("0%1").arg(seconds);
    else
        paddedSeconds = QString("%1").arg(seconds);

    return QString("%1:%2:%3.%4").arg(paddedHours, paddedMinutes, paddedSeconds).arg(milliseconds);
}

int Thumbnail::origin(const Video *video, const int &percent) const
{
    if(m_mode == thumb1)
        return 0;

    else if(m_mode == thumb2)
        switch(percent)
        {
            case 32: return 0;
            case 64: return 1 * video->width * _BPP;
            default: return 0;
        }

    else if(m_mode == thumb3)
        switch(percent)
        {
            case 24: return 0;
            case 48: return 1 * video->width * _BPP;
            case 72: return 2 * video->width * _BPP;
            default: return 0;
        }

    else if(m_mode == thumb4)
        switch(percent)
        {
            case 16: return 0;
            case 40: return 1 * video->width * _BPP;

            case 56: return 2 * video->width * _BPP * 1 * video->height;
            case 80: return 2 * video->width * _BPP * 1 * video->height + video->width * _BPP;
            default: return 0;
        }

    else if(m_mode == thumb6)
        switch(percent)
        {
            case 16: return 0;
            case 32: return 1 * video->width * _BPP;
            case 48: return 2 * video->width * _BPP;

            case 64: return 3 * video->width * _BPP * 1 * video->height;
            case 72: return 3 * video->width * _BPP * 1 * video->height + 1 * video->width * _BPP;
            case 88: return 3 * video->width * _BPP * 1 * video->height + 2 * video->width * _BPP;
            default: return 0;
        }

    else if(m_mode == thumb9)
        switch(percent)
        {
            case  8: return 0;
            case 16: return 1 * video->width * _BPP;
            case 32: return 2 * video->width * _BPP;

            case 40: return 3 * video->width * _BPP * 1 * video->height;
            case 48: return 3 * video->width * _BPP * 1 * video->height + 1 * video->width * _BPP;
            case 56: return 3 * video->width * _BPP * 1 * video->height + 2 * video->width * _BPP;

            case 72: return 3 * video->width * _BPP * 2 * video->height;
            case 80: return 3 * video->width * _BPP * 2 * video->height + 1 * video->width * _BPP;
            case 88: return 3 * video->width * _BPP * 2 * video->height + 2 * video->width * _BPP;
            default: return 0;
        }

    else if(m_mode == thumb12)
        switch(percent)
        {
            case  8: return 0;
            case 16: return 1 * video->width * _BPP;
            case 24: return 2 * video->width * _BPP;
            case 32: return 3 * video->width * _BPP;

            case 40: return 4 * video->width * _BPP * 1 * video->height;
            case 48: return 4 * video->width * _BPP * 1 * video->height + 1 * video->width * _BPP;
            case 56: return 4 * video->width * _BPP * 1 * video->height + 2 * video->width * _BPP;
            case 64: return 4 * video->width * _BPP * 1 * video->height + 3 * video->width * _BPP;

            case 72: return 4 * video->width * _BPP * 2 * video->height;
            case 80: return 4 * video->width * _BPP * 2 * video->height + 1 * video->width * _BPP;
            case 88: return 4 * video->width * _BPP * 2 * video->height + 2 * video->width * _BPP;
            case 96: return 4 * video->width * _BPP * 2 * video->height + 3 * video->width * _BPP;
            default: return 0;
        }

    return 0;
}
