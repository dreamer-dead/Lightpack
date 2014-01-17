#-------------------------------------------------
#
# Project created by QtCreator 2012-06-05T13:03:08
#
#-------------------------------------------------

QT       -= core gui

DESTDIR  = ../lib
TARGET   = prismatik-hooks
TEMPLATE = lib

include(../build-config.prf)

INCLUDEPATH += "$${DIRECTX_SDK_DIR}/Include"
               # ../zeromq/include

LIBS += -lshlwapi -ladvapi32 -luser32 -L"$${DIRECTX_SDK_DIR}/Lib/x86" -ldxguid #-LD:/System/Users/Tim/Projects/Lightpack/Software/zeromq -lzmq.dll
#QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas
QMAKE_LFLAGS_EXCEPTIONS_ON -= -mthreads
QMAKE_CXXFLAGS_EXCEPTIONS_ON -= -mthreads

CONFIG -= rtti

DEFINES += HOOKSDLL_EXPORTS UNICODE

CONFIG(msvc) {
    DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE
} else {
    QMAKE_CXXFLAGS = -std=c++11
    QMAKE_LFLAGS += -static
}

SOURCES += \
    hooks.cpp \
    ProxyFuncJmp.cpp \
    hooksutils.cpp \
    IPCContext.cpp \
    ProxyFuncVFTable.cpp \
    GAPIProxyFrameGrabber.cpp \
    DxgiFrameGrabber.cpp \
    Logger.cpp \
    D3D9FrameGrabber.cpp

HEADERS += \
    hooks.h \
    ../common/D3D10GrabberDefs.hpp \
    ../common/defs.h \
    ProxyFunc.hpp \
    ProxyFuncJmp.hpp \
    hooksutils.h \
    IPCContext.hpp \
    GAPISubstFunctions.hpp \
    ProxyFuncVFTable.hpp \
    res/logmessages.h \
    GAPIProxyFrameGrabber.hpp \
    DxgiFrameGrabber.hpp \
    Logger.hpp \
    LoggableTrait.hpp \
    ../common/BufferFormat.h \
    msvcstub.h \
    D3D9FrameGrabber.hpp
