#-------------------------------------------------
#
# Project created by QtCreator 2015-04-10T13:46:59
#
#-------------------------------------------------

QT       -= gui

TARGET = KaerHid
TEMPLATE = lib

DEFINES += KAERHID_LIBRARY

SOURCES += kaerhid.cc \
    ../../SAMVServer/samvprotocal.cpp

HEADERS += kaerhid.h\
    ../../SAMVServer/samvprotocal.h

#unix {
#    target.path = /usr/lib
#    INSTALLS += target
#}

win32 {
INCLUDEPATH += $$PWD/../include
#RC_FILE = qusb.rc

}
CONFIG += dll
LIBS += -lusb-1.0


headers.files = kaerhid.h
headers.path = $$PWD/../include/
target.path = $${QUSB_INSTALL_PREFIX}/lib
