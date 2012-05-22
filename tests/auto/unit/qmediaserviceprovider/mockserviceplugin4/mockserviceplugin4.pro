
load(qt_build_config)

TARGET = mockserviceplugin4
QT += multimedia-private
CONFIG += no_private_qt_headers_warning

PLUGIN_TYPE=mediaservice
DESTDIR = ../$${PLUGIN_TYPE}

load(qt_plugin)

HEADERS += ../mockservice.h
SOURCES += mockserviceplugin4.cpp
OTHER_FILES += mockserviceplugin4.json

target.path += $$[QT_INSTALL_TESTS]/tst_qmediaserviceprovider/$${PLUGIN_TYPE}
INSTALLS += target
