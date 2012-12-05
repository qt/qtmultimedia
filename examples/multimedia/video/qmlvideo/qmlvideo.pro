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

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/video/qmlvideo
INSTALLS += target

