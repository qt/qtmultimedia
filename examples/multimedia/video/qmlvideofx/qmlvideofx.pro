TEMPLATE = app
TARGET = qmlvideofx

QT += quick multimedia

SOURCES += filereader.cpp main.cpp
HEADERS += filereader.h trace.h

RESOURCES += qmlvideofx.qrc

include($$PWD/../snippets/performancemonitor/performancemonitordeclarative.pri)

maemo6: {
    DEFINES += SMALL_SCREEN_LAYOUT
    DEFINES += SMALL_SCREEN_PHYSICAL
}

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/video/qmlvideofx
INSTALLS += target
