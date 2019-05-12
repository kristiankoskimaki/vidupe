TARGET = Vidupe
TEMPLATE = app

QT += core gui widgets sql

QMAKE_LFLAGS += -Wl,--large-address-aware

HEADERS += \
    mainwindow.h \
    prefs.h \
    video.h \
    thumbnail.h \
    db.h \
    comparison.h

SOURCES += \
    mainwindow.cpp \
    video.cpp \
    thumbnail.cpp \
    phash.cpp \
    db.cpp \
    comparison.cpp \
    ssim.cpp

FORMS += \
    mainwindow.ui \
    comparison.ui

LIBS += \
    $$PWD/bin/libopencv_core345.dll \
    $$PWD/bin/libopencv_imgproc345.dll

RC_ICONS = vidupe16.ico

VERSION = 1.1
QMAKE_TARGET_PRODUCT = "Vidupe"
QMAKE_TARGET_DESCRIPTION = "Vidupe"
QMAKE_TARGET_COPYRIGHT = "Copyright \\251 2018-2019 Kristian Koskim\\344ki"

DEFINES += APP_VERSION=\\\"$$VERSION\\\"
DEFINES += APP_NAME=\"\\\"$$QMAKE_TARGET_PRODUCT\\\"\"
DEFINES += APP_COPYRIGHT=\"\\\"$$QMAKE_TARGET_COPYRIGHT\\\"\"

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

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
