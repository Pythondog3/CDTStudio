QT       += core

TARGET = AutoSVM
TEMPLATE = lib
CONFIG += plugin

DESTDIR = ../../../bin/Plugins

SOURCES += autosvminterface.cpp

HEADERS += autosvminterface.h

include(../../../Interfaces/Interfaces.pri)

OTHER_FILES += AutoSVM.json

unix{
QMAKE_CXXFLAGS += -std=c++0x
target.path = /usr/lib
INSTALLS += target
}

#opencv
include(../../../Tools/Config/link2opencv.pri)
