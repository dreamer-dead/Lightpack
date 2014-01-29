#-------------------------------------------------
#
# Project created by QtCreator 2011-09-09T12:09:24
#
#-------------------------------------------------

QT         += widgets network testlib

TARGET      = LightpackTests
DESTDIR     = bin

CONFIG     += console
CONFIG     -= app_bundle

TEMPLATE    = app

# QMake and GCC produce a lot of stuff
OBJECTS_DIR = stuff
MOC_DIR     = stuff
UI_DIR      = stuff
RCC_DIR     = stuff


DEFINES += SRCDIR=\\\"$$PWD/\\\"

LIBS += -L../lib -lprismatik-math -lgrab -lgtest
CONFIG(win32):LIBS+=-ladvapi32

INCLUDEPATH += ../src/ ../src/grab ../hooks ../grab/include ../math/include ../gtest/include
SOURCES += \
    LightpackApiTest.cpp \
    ../src/ApiServerSetColorTask.cpp \
    ../src/ApiServer.cpp \
    ../src/Settings.cpp \
    ../src/Plugin.cpp \
    ../src/LightpackPluginInterface.cpp \
    SettingsWindowMockup.cpp \
    GrabCalculationTest.cpp \
    lightpackmathtest.cpp \
    HooksTest.cpp \
    ../hooks/ProxyFuncJmp.cpp \
    ../hooks/hooksutils.cpp \
    ../hooks/ProxyFuncVFTable.cpp \
    ../hooks/Logger.cpp \
    TestsMain.cpp

HEADERS += \
    ../src/grab/include/calculations.hpp \
    ../common/defs.h \
    ../src/enums.hpp \
    ../src/ApiServerSetColorTask.hpp \
    ../src/ApiServer.hpp \
    ../src/debug.h \
    ../src/Settings.hpp \
    ../src/Plugin.hpp \
    ../src/LightpackPluginInterface.hpp \
    SettingsWindowMockup.hpp \
    ../math/include/PrismatikMath.hpp \
    ../hooks/ProxyFuncJmp.hpp \
    ../hooks/ProxyFunc.hpp \
    ../hooks/hooksutils.h \
    ../hooks/ProxyFuncVFTable.hpp \
    ../hooks/Logger.hpp

include(../src/qtsingleapplication/src/qtsingleapplication.pri)
