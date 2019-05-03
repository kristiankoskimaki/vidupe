#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget *parent) { Q_UNUSED (parent); }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) { Q_UNUSED (event); emit clicked(); }

};

#endif // CLICKABLELABEL_H
