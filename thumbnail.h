#ifndef THUMBNAIL_H
#define THUMBNAIL_H

#include <QVector>

enum modes { thumb1, thumb2, thumb3, thumb4, thumb6, thumb9, thumb12 };

class Thumbnail
{
public:
    Thumbnail(const int &mode = thumb4) { m_mode = mode; }

private:
    int m_mode = thumb4;
    QStringList m_modeNames =             { "1x1", "2x1", "3x1", "2x2", "3x2", "3x3", "4x3" };
    QVector< QVector<ushort> > m_layout = { {1,1}, {2,1}, {3,1}, {2,2}, {3,2}, {3,3}, {4,3} };    //{cols,rows}

    QVector< QVector<short> > m_capturePos = { { 48 },          //percent (of duration)
                                               { 32, 64 },
                                               { 24, 48, 72 },
                                               { 16, 40,    56, 80 },
                                               { 16, 32, 48,    64, 72, 88 },
                                               {  8, 16, 32,    40, 48, 56,     72, 80, 88 },
                                               {  8, 16, 24, 32,    40, 48, 56, 64,     72, 80, 88, 96 } };
public:
    int countModes() { return m_modeNames.count(); }
    QString modeName(const int mode) { return m_modeNames[mode]; }
    ushort cols() { return m_layout[m_mode][0]; }
    ushort rows() { return m_layout[m_mode][1]; }
    QVector<short> percentages() { return m_capturePos[m_mode]; }
};

#endif // THUMBNAIL_H
