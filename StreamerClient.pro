#-------------------------------------------------
#
# Project created by QtCreator 2016-04-23T08:03:35
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = StreamerClient
TEMPLATE = app


SOURCES += main.cpp\
        streamerclient.cpp

HEADERS  += streamerclient.h

FORMS    += streamerclient.ui
LIBS        +=-L/usr/local/lib -lopencv_core
