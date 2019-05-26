#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QDragEnterEvent>
#include <QMimeData>
#include "ui_mainwindow.h"
#include "video.h"

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() { deleteTemporaryFiles(); delete ui; }

private:
    Ui::MainWindow *ui;

    QVector<Video *> _videoList;
    QStringList _everyVideo;
    QStringList _rejectedVideos;
    QStringList _extensionList;

    Prefs _prefs;
    bool _userPressedStop = false;
    QString _previousRunFolders = QStringLiteral("");
    int _previousRunThumbnails = 0;

private slots:
    void deleteTemporaryFiles() const;
    void dragEnterEvent(QDragEnterEvent *event) { if(event->mimeData()->hasUrls()) event->acceptProposedAction(); }
    void dropEvent(QDropEvent *event);
    void loadExtensions();
    bool detectffmpeg() const;

    void setComparisonMode(const int &mode) { if(mode == _prefs._PHASH) ui->selectPhash->click(); else ui->selectSSIM->click(); ui->directoryBox->setFocus(); }
    void on_selectThumbnails_activated(const int &index) { _prefs._thumbnails = index; ui->directoryBox->setFocus(); }
    void on_selectPhash_clicked(const bool &checked) { if(checked) _prefs._comparisonMode = _prefs._PHASH; ui->directoryBox->setFocus(); }
    void on_selectSSIM_clicked(const bool &checked) { if(checked) _prefs._comparisonMode = _prefs._SSIM; ui->directoryBox->setFocus(); }
    void on_blocksizeCombo_activated(const int &index) { _prefs._ssimBlockSize = static_cast<int>(pow(2, index+1)); ui->directoryBox->setFocus(); }
    void on_differentDurationCombo_activated(const int &index) { _prefs._differentDurationModifier = index; ui->directoryBox->setFocus(); }
    void on_sameDurationCombo_activated(const int &index) { _prefs._sameDurationModifier = index; ui->directoryBox->setFocus(); }
    void on_thresholdSlider_valueChanged(const int &value) { ui->thresholdSlider->setValue(value); calculateThreshold(value); ui->directoryBox->setFocus(); }
    void calculateThreshold(const int &value);

    void on_browseFolders_clicked() const;
    void on_directoryBox_returnPressed() { on_findDuplicates_clicked(); }
    void on_findDuplicates_clicked();
    void findVideos(QDir &dir);
    void processVideos();
    void videoSummary();

    void addStatusMessage(const QString &message) const;
    void addVideo(const QString &filename) const;
    void removeVideo(Video *deleteMe);
};

#endif // MAINWINDOW_H
