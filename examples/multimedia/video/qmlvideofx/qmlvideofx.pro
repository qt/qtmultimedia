TEMPLATE = app
TARGET = qmlvideofx

QT += quick

LOCAL_SOURCES = filereader.cpp main.cpp
LOCAL_HEADERS = filereader.h trace.h

SOURCES += $$LOCAL_SOURCES
HEADERS += $$LOCAL_HEADERS

RESOURCES += qmlvideofx.qrc

SNIPPETS_PATH = ../snippets
include($$SNIPPETS_PATH/performancemonitor/performancemonitordeclarative.pri)

maemo6: {
    DEFINES += SMALL_SCREEN_LAYOUT
    DEFINES += SMALL_SCREEN_PHYSICAL
}

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/video/qmlvideofx
INSTALLS += target
