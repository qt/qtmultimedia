
load(qt_build_config)

TARGET = mockserviceplugin1
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
SOURCES += mockserviceplugin1.cpp
OTHER_FILES += mockserviceplugin1.json

target.path += $$[QT_INSTALL_TESTS]/tst_qmediaserviceprovider/$${PLUGIN_TYPE}
INSTALLS += target
