#include "video.h"

/* 1x1     2x1         3x1         2x2         3x2           3x3             4*3               4x4
  +---+ +---+---+ +---+---+---+ +---+---+ +---+---+---+ +---+---+---+ +---+---+---+---+ +---+---+---+---+
  |51%| |34%|68%| |25%|50%|75%| |20%|40%| |15%|30%|45%| |10%|20%|30%| | 8%|16%|24%|32%| | 6%|12%|18%|24%|
  +---+ +---+---+ +---+---+---+ +---+---+ +---+---+---+ +---+---+---+ +---+---+---+---+ +---+---+---+---+
                                |60%|80%| |60%|75%|90%| |40%|50%|60%| |40%|48%|56%|64%| |30%|36%|42%|48%|
                                +---+---+ +---+---+---+ +---+---+---+ +---+---+---+---+ +---+---+---+---+
                                                        |70%|80%|90%| |72%|80%|88%|96%| |54%|60%|66%|72%|
                                                        +---+---+---+ +---+---+---+---+ +---+---+---+---+
                                                                                        |78%|84%|90%|96%|
                                                                                        +---+---+---+---+ */

void Video::setMergedWidthAndHeight(ushort &mergedWidth, ushort &mergedHeight) const
{
    if(thumbnails == 1*1)
    {
        mergedWidth =  1 * width;
        mergedHeight = 1 * height;
    }
    else if(thumbnails == 2*1)
    {
        mergedWidth =  2 * width;
        mergedHeight = 1 * height;
    }
    else if(thumbnails == 3*1)
    {
        mergedWidth =  3 * width;
        mergedHeight = 1 * height;
    }
    else if(thumbnails == 2*2)
    {
        mergedWidth =  2 * width;
        mergedHeight = 2 * height;
    }
    else if(thumbnails == 3*2)
    {
        mergedWidth =  3 * width;
        mergedHeight = 2 * height;
    }
    else if(thumbnails == 3*3)
    {
        mergedWidth =  3 * width;
        mergedHeight = 3 * height;
    }
    else if(thumbnails == 4*3)
    {
        mergedWidth =  4 * width;
        mergedHeight = 3 * height;
    }
    else if(thumbnails == 4*4)
    {
        mergedWidth =  4 * width;
        mergedHeight = 4 * height;
    }
}

ushort Video::getSkipPercent() const
{
    if(thumbnails == 1*1)
        return 51;                      //51%
    else if(thumbnails == 2*1)
        return 34;                      //34%, 68%
    else if(thumbnails == 3*1)
        return 25;                      //25%, 50%, 75%
    else if(thumbnails == 2*2)
        return 20;                      //20%, 40%, 60%, 80%
    else if(thumbnails == 3*2)
        return 15;                      //15%, 30%, .... 90%
    else if(thumbnails == 3*3)
        return 10;                      //10%, 20%, .... 90%
    else if(thumbnails == 4*3)
        return 8;                       //8%, 16%, .... 96%
    else if(thumbnails == 4*4)
        return 6;                       //6%, 12%, .... 96%
    return 100;
}

QString Video::msToHHMSS(const qint64 &time) const
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

int Video::calculateOrigin(const short &percent) const
{
    //every case counts backwards from 100
    if(thumbnails == 1*1)
    {
        switch(percent)
        {
            case 49: return 0;  //upper left
        }
    }

    else if(thumbnails == 2*1)
    {
        switch(percent)
        {
            case 32: return 0;  //upper left
            case 66: return 1 * width * _BPP;   //upper right
        }
    }

    else if(thumbnails == 3*1)
    {
        switch(percent)
        {
            case 25: return 0;  //left
            case 50: return 1 * width * _BPP;   //center
            case 75: return 2 * width * _BPP;   //right
        }
    }

    else if(thumbnails == 2*2)
    {
        switch(percent)
        {
            case 20: return 0;  //upper left
            case 40: return 1 * width * _BPP;   //upper right

            case 60: return 2 * width * _BPP * 1 * height;     //lower left
            case 80: return 2 * width * _BPP * 1 * height + width * _BPP;  //lower right
        }
    }

    else if(thumbnails == 3*2)
    {
        switch(percent)
        {
            case 10: return 0;  //upper left
            case 25: return 1 * width * _BPP;   //upper center
            case 40: return 2 * width * _BPP;   //upper right

            case 55: return 3 * width * _BPP * 1 * height;     //lower left
            case 70: return 3 * width * _BPP * 1 * height + 1 * width * _BPP;  //lower center
            case 85: return 3 * width * _BPP * 1 * height + 2 * width * _BPP;  //lower right
        }
    }

    else if(thumbnails == 3*3)
    {
        switch(percent)
        {
            case 10: return 0;  //upper left
            case 20: return 1 * width * _BPP;   //upper center
            case 30: return 2 * width * _BPP;   //upper right

            case 40: return 3 * width * _BPP * 1 * height;     //middle left
            case 50: return 3 * width * _BPP * 1 * height + 1 * width * _BPP;  //middle center
            case 60: return 3 * width * _BPP * 1 * height + 2 * width * _BPP;  //middle right

            case 70: return 3 * width * _BPP * 2 * height;     //lower left
            case 80: return 3 * width * _BPP * 2 * height + 1 * width * _BPP;  //lower center
            case 90: return 3 * width * _BPP * 2 * height + 2 * width * _BPP;  //lower right
        }
    }

    else if(thumbnails == 4*3)
    {
        switch(percent)
        {
            case  4: return 0;  //upper left
            case 12: return 1 * width * _BPP;   //upper left of center
            case 20: return 2 * width * _BPP;   //upper right of center
            case 28: return 3 * width * _BPP;   //upper right

            case 36: return 4 * width * _BPP * 1 * height;     //middle left
            case 44: return 4 * width * _BPP * 1 * height + 1 * width * _BPP;  //middle left of center
            case 52: return 4 * width * _BPP * 1 * height + 2 * width * _BPP;  //middle right of center
            case 60: return 4 * width * _BPP * 1 * height + 3 * width * _BPP;  //middle right

            case 68: return 4 * width * _BPP * 2 * height;      //lower left
            case 76: return 4 * width * _BPP * 2 * height + 1 * width * _BPP;  //lower left of center
            case 84: return 4 * width * _BPP * 2 * height + 2 * width * _BPP;  //lower right of center
            case 92: return 4 * width * _BPP * 2 * height + 3 * width * _BPP;  //lower right
        }
    }

    else if(thumbnails == 4*4)
    {
        switch(percent)
        {
            case  6: return 0;  //upper left
            case 10: return 1 * width * _BPP;   //upper left of center
            case 16: return 2 * width * _BPP;   //upper right of center
            case 22: return 3 * width * _BPP;   //upper right

            case 28: return 4 * width * _BPP * 1 * height;     //upper middle left
            case 34: return 4 * width * _BPP * 1 * height + 1 * width * _BPP;  //upper middle left of center
            case 40: return 4 * width * _BPP * 1 * height + 2 * width * _BPP;  //upper middle right of center
            case 46: return 4 * width * _BPP * 1 * height + 3 * width * _BPP;  //upper middle right

            case 52: return 4 * width * _BPP * 2 * height;      //lower middle left
            case 58: return 4 * width * _BPP * 2 * height + 1 * width * _BPP;  //lower middle left of center
            case 64: return 4 * width * _BPP * 2 * height + 2 * width * _BPP;  //lower middle right of center
            case 70: return 4 * width * _BPP * 2 * height + 3 * width * _BPP;  //lower middle right

            case 76: return 4 * width * _BPP * 3 * height;      //lower left
            case 82: return 4 * width * _BPP * 3 * height + 1 * width * _BPP;  //lower left of center
            case 88: return 4 * width * _BPP * 3 * height + 2 * width * _BPP;  //lower right of center
            case 94: return 4 * width * _BPP * 3 * height + 3 * width * _BPP;  //lower right
        }
    }

    return 0;
}
