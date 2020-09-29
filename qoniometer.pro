#-------------------------------------------------
#
# Project created by QtCreator 2016-02-28T18:07:12
#
#-------------------------------------------------

QT       += core gui multimedia widgets dbus

TARGET = qoniometer
TEMPLATE = app

CONFIG += c++11
CONFIG += optimize_full
QMAKE_CXXFLAGS += -Wno-deprecated-declarations


SOURCES += main.cpp \
        pulseaudiomonitor.cpp \
        widget.cpp

HEADERS  += widget.h \
        pulseaudiomonitor.h \
        inlinehsv.h

LIBS += -lpulse-simple -lpulse
