
load(qt_build_config)

TARGET = mockserviceplugin2
QT += multimedia-private
CONFIG += no_private_qt_headers_warning

PLUGIN_TYPE=mediaservice
DESTDIR = ../$${PLUGIN_TYPE}
win32 {
    CONFIG(debug, debug|release) {
        DESTDIR = ../debug/$${PLUGIN_TYPE}
    } else {
        DESTDIR = ../release/$${PLUGIN_TYPE}
    }
}

load(qt_plugin)

HEADERS += ../mockservice.h
SOURCES += mockserviceplugin2.cpp
OTHER_FILES += mockserviceplugin2.json

target.path += $$[QT_INSTALL_TESTS]/tst_qmediaserviceprovider/$${PLUGIN_TYPE}
INSTALLS += target
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
