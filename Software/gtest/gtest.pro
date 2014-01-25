DESTDIR  = ../lib
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= app_bundle
CONFIG -= qt

CONFIG(gcc) {
    QMAKE_CXXFLAGS += -isystem $${PWD}/include
    QMAKE_CXXFLAGS += -g -Wall -Wextra -pthread
} else {
    INCLUDEPATH += ./include
}

# All Google Test headers.  Usually you shouldn't change this
# definition.
HEADERS = ./include/gtest/*.h \
          ./include/gtest/internal/*.h \
          ./src/*.h

SOURCES = ./src/gtest-all.cc

