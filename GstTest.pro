QT       += core gui network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    GstreamerThread.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    GstreamerThread.h \
    MainWindow.h

FORMS += \
    MainWindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DEFINES += ARCH='\\"64-Bit\\"'

INCLUDEPATH += "C:/gstreamer/1.0/msvc_x86_64/include/gstreamer-1.0"
INCLUDEPATH += "C:/gstreamer/1.0/msvc_x86_64/include/glib-2.0"
INCLUDEPATH += "C:/gstreamer/1.0/msvc_x86_64/lib/glib-2.0/include"
INCLUDEPATH += "C:/gstreamer/1.0/msvc_x86_64/include"

LIBS += -LC:/gstreamer/1.0/msvc_x86_64/bin
LIBS += -LC:/gstreamer/1.0/msvc_x86_64/lib/gstreamer-1.0
LIBS += -LC:/gstreamer/1.0/msvc_x86_64/lib


LIBS += -lgstapp-1.0 -lgstbase-1.0 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lintl \
-lglib-2.0 -lgstallocators-1.0 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 -lgstgl-1.0 -lgstvideo-1.0 \
-lgstbase-1.0 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0 \
        -lavformat -lavcodec -lavutil -lffi -lgmodule-2.0

INCLUDEPATH += "C:\Users\ChrisTsiao\Software\Libraries\ffmpeg\lib"
INCLUDEPATH += "C:\Users\ChrisTsiao\Software\Libraries\ffmpeg\bin"
INCLUDEPATH += "C:\gstreamer\1.0\msvc_x86_64\lib\gstreamer-1.0"







# --------------------------------- PRE PROCESSOR VARIABLES ---------------------------------
# Use this for GPU accelerated h264 decoding. Strangely the d3d11h264dec uses MORE CPU than avdec_h264
# DEFINES += USE_GPU_ACCEL
























