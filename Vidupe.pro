#-------------------------------------------------
#
# Project created by QtCreator 2018-08-26T11:24:06
#
#-------------------------------------------------

QT += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Vidupe
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_LFLAGS += -Wl,--large-address-aware

SOURCES += \
    mainwindow.cpp \
    video.cpp \
    thumbnail.cpp \
    phash.cpp \
    db.cpp \
    comparison.cpp \
    ssim.cpp

HEADERS += \
    mainwindow.h \
    prefs.h \
    video.h \
    db.h \
    comparison.h \
    clickablelabel.h

FORMS += \
    mainwindow.ui \
    comparison.ui

RC_ICONS = vidupe16.ico

LIBS += \
    $$PWD/bin/libopencv_core341.dll \
    $$PWD/bin/libopencv_imgproc341.dll

#How to compile Vidupe:
    #Qt5.xx (https://www.qt.io/) MingW-32 is the default compiler and was used for Vidupe development
    #If compilation fails, click on the computer icon in lower left corner of Qt Creator and select a kit

    #OpenCV 3.xx (32 bit) (https://www.opencv.org/)
    #Compiling OpenCV with MingW can be hard, so download binaries from https://github.com/huihut/OpenCV-MinGW-Build
    #put OpenCV \bin folder in source folder (only libopencv_core and libopencv_imgproc dll files are needed)
    #put OpenCV \opencv2 folder in source folder (contains the header files)
    #add path to \bin folder (Projects -> Build Environment -> Details -> Path) (crash on start without this)

    #FFmpeg 4.xx (https://ffmpeg.org/)
    #ffmpeg.exe must be in same folder where Vidupe.exe is generated (or any folder in %PATH%)

    #extensions.ini must be in folder where Vidupe.exe is generated (\build-Vidupe-Desktop_Qt_5___MinGW_32bit-Debug\debug)
