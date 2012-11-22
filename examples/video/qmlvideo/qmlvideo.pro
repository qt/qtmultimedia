TEMPLATE = app
TARGET = qmlvideo

LOCAL_SOURCES = main.cpp
LOCAL_HEADERS = trace.h

SOURCES += $$LOCAL_SOURCES
HEADERS += $$LOCAL_HEADERS
RESOURCES += qmlvideo.qrc

SNIPPETS_PATH = ../snippets
include($$SNIPPETS_PATH/performancemonitor/performancemonitordeclarative.pri)

include(qmlapplicationviewer/qmlapplicationviewer.pri)

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/video/qmlvideo
sources.files = $$LOCAL_SOURCES $$LOCAL_HEADERS $$RESOURCES *.pro images qmlapplicationviewer qmlvideo.png qml qmlvideo.svg
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/video/qmlvideo
INSTALLS += target sources

