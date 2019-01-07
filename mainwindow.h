#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QMimeData>
#include "video.h"
#include "prefs.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QVector<Video *> _videoList;
    QStringList _rejectedVideos;
    QStringList _extensionList;
    Prefs _prefs;
    bool _userPressedStop = false;

private slots:
    void deleteTemporaryFiles() const;
    void dragEnterEvent(QDragEnterEvent *event) { if(event->mimeData()->hasUrls()) event->acceptProposedAction(); }
    void dropEvent(QDropEvent *event);
    void loadExtensions();
    bool detectffmpeg() const;

    void on_selectThumbnails_activated(const int &index);
    void on_selectPhash_clicked(const bool &checked) { if(checked) _prefs._ComparisonMode = _prefs._PHASH; }
    void on_selectSSIM_clicked(const bool &checked) { if(checked) _prefs._ComparisonMode = _prefs._SSIM; }
    void on_blocksizeCombo_activated(const int &index) { _prefs._ssimBlockSize = static_cast<short>(pow(2, index+1)); }
    void on_differentDurationCombo_activated(const int &index) { _prefs._differentDurationModifier = static_cast<short>(index); }
    void on_sameDurationCombo_activated(const int &index) { _prefs._sameDurationModifier = static_cast<short>(index); }
    void on_thresholdSlider_valueChanged(const int &value) { calculateThreshold(value); }
    void calculateThreshold(const int &value);

    void on_browseFolders_clicked();
    void on_directoryBox_returnPressed() { on_findDuplicates_clicked(); }
    void on_findDuplicates_clicked();
    void findVideos(QDir &dir, QStringList &everyVideo) const;
    void processVideos(const QStringList &everyVideo);

    void addStatusMessage(const QString &message) const;
    void addVideo(const QString &filename) const;
    void removeVideo(Video *deleteMe);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
