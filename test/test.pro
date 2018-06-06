QT += core

QT-=gui
CONFIG += console

TEMPLATE = app


TARGET = test

linux-g++{

LIBS += -L$$PWD -lusb-1.0

}

linux-arm-gnueabi-g++{
    LIBS += -L$$PWD/arm -lusb-1.0
}



SOURCES += main.cc \
    kaerhid.cc \
    samvprotocal.cpp

HEADERS += \
    kaerhid.h \
    samvprotocal.h

