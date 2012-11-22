TEMPLATE = app
TARGET = qmlvideofx

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

include(qmlapplicationviewer/qmlapplicationviewer.pri)

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/video/qmlvideofx
sources.files = $$LOCAL_SOURCES $$LOCAL_HEADERS $$RESOURCES *.pro images qmlapplicationviewer qmlvideofx.png shaders qml qmlvideofx.svg
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/video/qmlvideofx
INSTALLS += target sources
