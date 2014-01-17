#-------------------------------------------------
#
# Project created by QtCreator 2013-06-11T16:47:52
#
#-------------------------------------------------

QT       += widgets

DESTDIR = ../lib
TARGET = grab
TEMPLATE = lib
CONFIG += staticlib

include(../build-config.prf)

LIBS += -lprismatik-math

INCLUDEPATH += ./include ../src ../math/include

HEADERS += \
    include/calculations.hpp \
    include/TimeredGrabber.hpp \
    include/QtGrabberEachWidget.hpp \
    include/QtGrabber.hpp \
    include/IGrabber.hpp \
    include/GrabberBase.hpp

SOURCES += \
    calculations.cpp \
    TimeredGrabber.cpp \
    QtGrabberEachWidget.cpp \
    QtGrabber.cpp \
    GrabberBase.cpp

CONFIG(msvc) {
    DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE
}

win32 {
    INCLUDEPATH += "$${DIRECTX_SDK_DIR}/Include"
    HEADERS += \
            include/D3D9Grabber.hpp \
            include/D3D10Grabber.hpp \
            include/WinAPIGrabberEachWidget.hpp \
            include/WinAPIGrabber.hpp
    SOURCES += \
            D3D9Grabber.cpp \
            D3D10Grabber.cpp \
            WinAPIGrabberEachWidget.cpp \
            WinAPIGrabber.cpp
}

macx {
    #QMAKE_LFLAGS += -F/System/Library/Frameworks

    INCLUDEPATH += /System/Library/Frameworks

    #LIBS += \
    #        -framework CoreGraphics
    #        -framework CoreFoundation

    HEADERS += \
            include/MacOSGrabber.hpp

    SOURCES += \
            MacOSGrabber.cpp

    #QMAKE_MAC_SDK = macosx10.8
}

unix:!macx {
    HEADERS += \
            include/X11Grabber.hpp

    SOURCES += \
            X11Grabber.cpp
}
