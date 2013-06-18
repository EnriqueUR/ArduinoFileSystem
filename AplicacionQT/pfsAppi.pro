#-------------------------------------------------
#
# Project created by QtCreator 2013-05-28T14:13:23
#
#-------------------------------------------------

QT       += core gui

TARGET = pfsAppi
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    customqlistwidget.cpp \
    pfsfolder.cpp \
    pfsfile.cpp \
    diskfunctions.cpp

HEADERS  += mainwindow.h \
    customqlistwidget.h \
    pfsfolder.h \
    pfsfile.h \
    SdPfsStructs.h \
    diskfunctions.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc
