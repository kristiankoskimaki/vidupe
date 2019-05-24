#include "mainwindow.h"
#include "comparison.h"
#include <QFileDialog>
#include <QDirIterator>
#include <QProcess>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
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
    ui->statusBox->append(QStringLiteral("%1 %2").arg(APP_NAME, APP_VERSION));
    ui->statusBox->append(QStringLiteral("%1").arg(APP_COPYRIGHT).replace("\xEF\xBF\xBD ", QStringLiteral("© "))
                                                                 .replace("\xEF\xBF\xBD",  QStringLiteral("ä")));
    ui->statusBox->append(QStringLiteral("Licensed under GNU General Public License\n"));

    deleteTemporaryFiles();
    loadExtensions();
    detectffmpeg();
    calculateThreshold(ui->thresholdSlider->sliderPosition());

    ui->blocksizeCombo->addItems( { QStringLiteral("2"), QStringLiteral("4"),
                                    QStringLiteral("8"), QStringLiteral("16") } );
    ui->blocksizeCombo->setCurrentIndex(3);
    Thumbnail thumb;
    for(int i=0; i<thumb.countModes(); i++)
        ui->selectThumbnails->addItem(thumb.modeName(i));
    ui->selectThumbnails->setCurrentIndex(3);

    for(int i=0; i<=9; i++)
    {
        ui->differentDurationCombo->addItem(QStringLiteral("%1").arg(i));
        ui->sameDurationCombo->addItem(QStringLiteral("%1").arg(i));
    }

    ui->differentDurationCombo->setCurrentIndex(4);
    ui->sameDurationCombo->setCurrentIndex(1);

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
        if(dir.dirName().compare(QStringLiteral("Vidupe-")) == 1)
            dir.removeRecursively();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QString fileName = event->mimeData()->urls().first().toLocalFile();
    const QFileInfo file(fileName);
    if(file.isDir())
        ui->directoryBox->insert(QStringLiteral(";%1").arg(QDir::toNativeSeparators(fileName)));
}

void MainWindow::loadExtensions()
{
    QFile file(QStringLiteral("%1/extensions.ini").arg(QApplication::applicationDirPath()));
    if(file.open(QIODevice::ReadOnly))
    {
        addStatusMessage(QStringLiteral("Supported file extensions:"));
        QTextStream text(&file);
        while(!text.atEnd())
        {
            const QString line = text.readLine();
            if(line.startsWith(QStringLiteral(";")))
                continue;
            else if(!line.isEmpty())
            {
                const QStringList extensions = line.split(QStringLiteral(" "));
                QString foundTheseExtensions;
                for(auto ext : extensions)
                {
                    _extensionList << ext;
                    foundTheseExtensions.append(ext.remove(QStringLiteral("*")) + QStringLiteral(" "));
                }
                addStatusMessage(foundTheseExtensions);
            }
        }
        file.close();
    }
    else
        addStatusMessage(QStringLiteral("Error: extensions.ini not found. No video file will be searched."));
}

bool MainWindow::detectffmpeg() const
{
    QProcess ffmpeg;
    ffmpeg.setProcessChannelMode(QProcess::MergedChannels);
    ffmpeg.start(QStringLiteral("ffmpeg"));
    ffmpeg.waitForFinished();

    if(ffmpeg.readAllStandardOutput().isEmpty())
    {
        addStatusMessage(QStringLiteral("Error: FFmpeg not found. Download it from https://ffmpeg.org/"));
        return false;
    }
    return true;
}

void MainWindow::calculateThreshold(const int &value)
{
    const int differentBits = 64 - value;           //two video's hashes may only differ this many bits
    const int percentage = 100 * value / 64;        //to be considered having the the same content
    const QString thresholdMessage = QStringLiteral(
                "Threshold: %1% (%2/64 bits may differ)\n"
                "Default is:  89% (7/64 bits may differ)\n"
                "Smaller: less strict, can match different videos (false positive)\n"
                "Larger:  more strict, can miss identical videos (false negative)").arg(percentage).arg(differentBits);
    ui->thresholdSlider->setToolTip(thresholdMessage);
    _prefs._thresholdPhash = differentBits;
    _prefs._thresholdSSIM = value / 64.0;
}

void MainWindow::on_browseFolders_clicked() const
{
    QString dir = QFileDialog::getExistingDirectory(nullptr, tr("Open Directory"), QStringLiteral("/home"),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(dir.isEmpty())
        return;
    dir = QStringLiteral(";%1").arg(QDir::toNativeSeparators(dir));
    ui->directoryBox->setText(ui->directoryBox->text().append(dir));
}

void MainWindow::on_findDuplicates_clicked()
{
    if(_extensionList.isEmpty())
    {
        addStatusMessage(QStringLiteral("Error: No extensions found in extensions.ini. No video file will be searched."));
        return;
    }
    if(!detectffmpeg())
        return;

    if(ui->findDuplicates->text() == QLatin1String("Stop"))     //pressing "find duplicates" button will morph into a
    {                                           //stop button. a lengthy search can thus be stopped and
        _userPressedStop = true;                //those videos already processed are compared w/each other
        return;
    }
    else
    {
        ui->findDuplicates->setText(QStringLiteral("Stop"));
        _userPressedStop = false;
    }

    //search for new videos only if folder or thumbnail settings have changed
    const QString foldersToSearch = ui->directoryBox->text();
    if(foldersToSearch != _previousRunFolders || _prefs._thumbnails != _previousRunThumbnails)
    {
        for(const auto &video : _videoList)     //new search: delete videos from previous search
            delete video;
        _videoList.clear();

        ui->statusBox->append(QStringLiteral("\nSearching for videos..."));
        ui->processedFiles->setVisible(false);
        ui->progressBar->setVisible(false);

        QStringList directories = foldersToSearch.split(QStringLiteral(";"));

        //add all video files from entered paths to list
        QStringList everyVideo;
        for(int i=0; i<directories.length(); i++)
        {
            if(!directories.value(i).isEmpty())
            {
                QDir dir = directories.value(i).remove(QStringLiteral("\""));
                if(dir.exists())
                    findVideos(dir, everyVideo);
                else
                    addStatusMessage(QStringLiteral("Cannot find directory: %1").arg(QDir::toNativeSeparators(dir.path())));
            }
        }

        ui->statusBox->append(QStringLiteral("Found %1 video file(s):").arg(everyVideo.count()));
        everyVideo.sort();
        processVideos(everyVideo);
    }

    if(_videoList.count() > 1)      //very last thing to do in this file: start the comparison
    {
        Comparison comparison(_videoList, _prefs, *this);
        QFuture<void> future = QtConcurrent::run(&comparison, &Comparison::reportMatchingVideos);   //run in background
        comparison.exec();          //open dialog, but if it is closed while reportMatchingVideos() still running...

        QApplication::setOverrideCursor(Qt::WaitCursor);
        future.waitForFinished();   //...must wait until finished (crash when going out of scope destroys instance)
        QApplication::restoreOverrideCursor();

        _previousRunFolders = foldersToSearch;                  //videos are still held in memory until
        _previousRunThumbnails = _prefs._thumbnails;            //folders to search or thumbnail mode are changed
    }

    ui->findDuplicates->setText(QStringLiteral("Find duplicates"));
}

void MainWindow::findVideos(QDir &dir, QStringList &everyVideo) const
{
    dir.setNameFilters(_extensionList);
    QDirIterator iter(dir, QDirIterator::Subdirectories);
    while(iter.hasNext())
    {
        if(_userPressedStop)
            return;
        const QFile file(iter.next());
        const QString filename = file.fileName();

        bool duplicate = false;                 //don't want duplicates of same file
        for(const auto &alreadyAddedFile : everyVideo)
            if(filename.toLower() == alreadyAddedFile.toLower())
            {
                duplicate = true;
                break;
            }
        if(!duplicate)
            everyVideo << filename;

        ui->statusBar->showMessage(QStringLiteral("%1").arg(QDir::toNativeSeparators(filename)), 10);
        QApplication::processEvents();
    }
}

void MainWindow::processVideos(const QStringList &everyVideo)
{
    const int thumbnails = _prefs._thumbnails;
    const int numberOfVideos = everyVideo.count();
    if(numberOfVideos > 0)
    {
        ui->processedFiles->setVisible(true);
        ui->processedFiles->setText(QStringLiteral("0/%1").arg(numberOfVideos));
        ui->statusBar->clearMessage();
        ui->progressBar->setVisible(true);
        ui->progressBar->setValue(0);
        ui->progressBar->setMaximum(everyVideo.count());
    }
    else return;

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

    if(_rejectedVideos.empty())
        addStatusMessage(QStringLiteral("%1 intact video(s) were found").arg(everyVideo.count()));
    else
    {
        addStatusMessage(QStringLiteral("%1 intact video(s) out of %2 total").arg(_videoList.count())
                                                                             .arg(everyVideo.count()));
        addStatusMessage(QStringLiteral("\nThe following %1 video(s) could not be added due to errors:")
                         .arg(_rejectedVideos.count()));
        for(const auto &filename : _rejectedVideos)
            addStatusMessage(filename);

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
    ui->processedFiles->setText(QStringLiteral("%1/%2").arg(ui->progressBar->value()).arg(ui->progressBar->maximum()));
}

void MainWindow::removeVideo(Video *deleteMe)
{
    addStatusMessage(QStringLiteral("[%1] ERROR reading %2").arg(QTime::currentTime().toString(),
                                                                 QDir::toNativeSeparators(deleteMe->filename)));
    ui->progressBar->setValue(ui->progressBar->value() + 1);
    ui->processedFiles->setText(QStringLiteral("%1/%2").arg(ui->progressBar->value()).arg(ui->progressBar->maximum()));
    _rejectedVideos << QDir::toNativeSeparators(deleteMe->filename);
    _videoList.removeAll(deleteMe);
    delete deleteMe;
}
