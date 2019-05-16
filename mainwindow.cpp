#include "mainwindow.h"
#include "comparison.h"
#include <QFileDialog>
#include <QDirIterator>
#include <QProcess>
#include <QThreadPool>
#include <QScrollBar>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    ui->statusBox->append(QString("%1 %2").arg(APP_NAME, APP_VERSION));
    ui->statusBox->append(QString("%1").arg(APP_COPYRIGHT).replace("\xEF\xBF\xBD ", "© ").replace("\xEF\xBF\xBD", "ä"));
    ui->statusBox->append("Licensed under GNU General Public License\n");

    deleteTemporaryFiles();
    loadExtensions();
    detectffmpeg();
    calculateThreshold(ui->thresholdSlider->sliderPosition());

    ui->blocksizeCombo->addItems( { "2", "4", "8", "16" } );
    ui->blocksizeCombo->setCurrentIndex(3);

    Thumbnail thumb;
    for(ushort i=0; i<thumb.countModes(); i++)
        ui->selectThumbnails->addItem(thumb.modeName(i));
    ui->selectThumbnails->setCurrentIndex(3);

    for(ushort i=0; i<=9; i++)
    {
        QString index = QString("%1").arg(i);
        ui->differentDurationCombo->addItem(index);
        ui->sameDurationCombo->addItem(index);
    }

    ui->differentDurationCombo->setCurrentIndex(5);
    ui->sameDurationCombo->setCurrentIndex(5);

    ui->directoryBox->setFocus();
    ui->browseFolders->setIcon(ui->browseFolders->style()->standardIcon(QStyle::SP_DirOpenIcon));

    ui->processedFiles->setVisible(false);
    ui->progressBar->setVisible(false);
    ui->mainToolBar->setVisible(false);
}

void MainWindow::deleteTemporaryFiles() const
{
    QDir tempDir = QDir::tempPath();    //QTemporaryDir remains if program force quit
    tempDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QDirIterator iter(tempDir);
    while(iter.hasNext())
    {
        QDir dir = iter.next();
        if(dir.dirName().compare("Vidupe-") == 1)
            dir.removeRecursively();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QString fileName = event->mimeData()->urls().first().toLocalFile();
    const QFileInfo file(fileName);
    if(file.isDir())
        ui->directoryBox->insert(";" + QDir::toNativeSeparators(fileName));
}

void MainWindow::loadExtensions()
{
    QFile file(QString("%1/extensions.ini").arg(QApplication::applicationDirPath()));
    if(file.open(QIODevice::ReadOnly))
    {
        addStatusMessage("Supported file extensions:");
        QTextStream text(&file);
        while(!text.atEnd())
        {
            const QString line = text.readLine();
            if(line.startsWith(";"))
                continue;
            else if(!line.isEmpty())
            {
                const QStringList extensions = line.split(" ");
                QString foundTheseExtensions;
                for(int i=0; i<extensions.size(); i++)
                {
                    QString ext = QString(extensions[i]).remove("*").remove(".").prepend("*.");
                    _extensionList << ext;
                    foundTheseExtensions.append(ext.remove("*") + " ");
                }
                addStatusMessage(foundTheseExtensions);
            }
        }
        file.close();
    }
    else
        addStatusMessage("Error: extensions.ini not found. No video file will be searched.");
}

bool MainWindow::detectffmpeg() const
{
    QProcess ffmpeg;
    ffmpeg.setProcessChannelMode(QProcess::MergedChannels);
    ffmpeg.start("ffmpeg");
    ffmpeg.waitForFinished();

    if(ffmpeg.readAllStandardOutput() == "")
    {
        addStatusMessage("Error: FFmpeg not found. Download it from https://ffmpeg.org/");
        return false;
    }
    return true;
}

void MainWindow::calculateThreshold(const int &value)
{
    const int differentBits = 64 - value;           //two video's hashes may only differ this many bits
    const int percentage = 100 * value / 64;        //to be considered having the the same content
    const QString thresholdMessage = QString(
                "Threshold: %1% (%2/64 bits may differ)\n"
                "Default is:  89% (7/64 bits may differ)\n"
                "Smaller: less strict, can match different videos (false positive)\n"
                "Larger:  more strict, can miss identical videos (false negative)").arg(percentage).arg(differentBits);
    ui->thresholdSlider->setToolTip(thresholdMessage);
    _prefs._thresholdPhash = static_cast<short>(differentBits);
    _prefs._thresholdSSIM = static_cast<double>(value) / 64;
}

void MainWindow::on_browseFolders_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "/home",
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(dir == "")
        return;
    dir = QString(";%1").arg(QDir::toNativeSeparators(dir));
    ui->directoryBox->setText(ui->directoryBox->text().append(dir));
}

void MainWindow::on_findDuplicates_clicked()
{
    if(_extensionList.isEmpty())
    {
        addStatusMessage("Error: No extensions found in extensions.ini. No video file will be searched.");
        return;
    }
    if(!detectffmpeg())
        return;

    if(ui->findDuplicates->text() == "Stop")    //pressing "find duplicates" button will morph into a
    {                                           //stop button. a lengthy search can thus be stopped and
        _userPressedStop = true;                //those videos already processed are compared w/each other
        return;
    }
    else
    {
        ui->findDuplicates->setText("Stop");
        _userPressedStop = false;
    }

    //search for new videos only if folder or thumbnail settings have changed
    const QString foldersToSearch = ui->directoryBox->text();
    if(foldersToSearch != _previousRunFolders || _prefs._thumbnails != _previousRunThumbnails)
    {
        for(int i=0; i<_videoList.count(); i++)     //new search: delete videos from previous search
            delete _videoList[i];
        _videoList.clear();

        ui->statusBox->append("\nSearching for videos...");
        ui->processedFiles->setVisible(false);
        ui->progressBar->setVisible(false);

        //QStringList directories = ui->directoryBox->text().split(";");
        QStringList directories = foldersToSearch.split(";");

        //add all video files from entered paths to list
        QStringList everyVideo;
        for(int i=0; i<directories.length(); i++)
        {
            if(directories.value(i) != "")
            {
                QDir dir = directories.value(i).remove('"');
                if(dir.exists())
                    findVideos(dir, everyVideo);
                else
                    addStatusMessage(QString("Cannot find directory: %1").arg(QDir::toNativeSeparators(dir.path())));
            }
        }

        ui->statusBox->append(QString("Found %1 video file(s):").arg(everyVideo.count()));
        everyVideo.sort();
        processVideos(everyVideo);
    }

    if(_videoList.count() > 0)      //very last thing to do in this file: start the comparison
    {
        Comparison comparisonWnd(_videoList, _prefs, *this);
        comparisonWnd.exec();
    }

    _previousRunFolders = foldersToSearch;
    _previousRunThumbnails = _prefs._thumbnails;
    ui->findDuplicates->setText("Find duplicates");
}

void MainWindow::findVideos(QDir &dir, QStringList &everyVideo) const
{
    dir.setNameFilters(_extensionList);
    QDirIterator iter(dir, QDirIterator::Subdirectories);
    while(iter.hasNext())
    {
        const QFile file(iter.next());
        const QString filename = file.fileName();
        bool alreadyAdded = false;              //don't want duplicates of same file
        const int numberOfVideos = everyVideo.count();
        for(int i=0; i<numberOfVideos; i++)
        {
            if(everyVideo[i].toLower() == filename.toLower())
                alreadyAdded = true;
        }
        if(!alreadyAdded)
            everyVideo << filename;
        ui->statusBar->showMessage(QString("%1").arg(QDir::toNativeSeparators(filename)), 10);
        QApplication::processEvents();
    }
}

void MainWindow::processVideos(const QStringList &everyVideo)
{
    const int numberOfVideos = everyVideo.count();
    if(numberOfVideos > 0)
    {
        ui->processedFiles->setVisible(true);
        ui->processedFiles->setText(QString("0/%1").arg(numberOfVideos));
        ui->statusBar->showMessage("");
        ui->progressBar->setVisible(true);
        ui->progressBar->setValue(0);
        ui->progressBar->setMaximum(everyVideo.count());
    }
    const ushort thumbnails = _prefs._thumbnails;

    QThreadPool threadPool;
    for(int i=0; i<numberOfVideos; i++)
    {
        if(_userPressedStop)
        {
            threadPool.clear();
            break;
        }
        if(threadPool.activeThreadCount() == threadPool.maxThreadCount())
        {
            QApplication::processEvents();      //avoid blocking signals in event loop
            i--;
            continue;
        }

        auto *videoTask = new Video(*this, everyVideo[i], numberOfVideos, thumbnails);
        videoTask->setAutoDelete(false);
        threadPool.start(videoTask);
        _videoList << videoTask;
    }
    threadPool.waitForDone();
    QApplication::processEvents();    //process signals from last threads

    addStatusMessage(QString("%1 intact video(s) out of %2 total").arg(_videoList.count()).arg(everyVideo.count()));
    if(!_rejectedVideos.empty())
    {
        addStatusMessage(QString("\nThe following %1 video(s) could not be added due to errors:")
                         .arg(_rejectedVideos.count()));
        for(int i=0; i<_rejectedVideos.count(); i++)
            addStatusMessage(_rejectedVideos[i]);
    }
    _rejectedVideos.clear();
}

void MainWindow::addStatusMessage(const QString &message) const
{
    ui->statusBox->append(message);
    ui->statusBox->repaint();
    ui->statusBox->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
}

void MainWindow::addVideo(const QString &filename) const
{
    addStatusMessage(filename);
    ui->progressBar->setValue(ui->progressBar->value() + 1);
    ui->processedFiles->setText(QString("%1/%2").arg(ui->progressBar->value()).arg(ui->progressBar->maximum()));
}

void MainWindow::removeVideo(Video *deleteMe)
{
    _rejectedVideos << QDir::toNativeSeparators(deleteMe->filename);
    _videoList.removeAll(deleteMe);
    addStatusMessage(QString("[%1] ERROR reading %2").arg(QTime::currentTime().toString(),
                                                          QDir::toNativeSeparators(deleteMe->filename)));
    ui->progressBar->setValue(ui->progressBar->value() + 1);
    ui->processedFiles->setText(QString("%1/%2").arg(ui->progressBar->value()).arg(ui->progressBar->maximum()));
}
