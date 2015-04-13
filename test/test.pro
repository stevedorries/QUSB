TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cc


LIBS += -lKaerHid
#include(deployment.pri)
#qtcAddDeployment()

HEADERS += \
    ../KaerHid/kaerhid.h

