
load(qt_build_config)

TARGET = mockserviceplugin3
QT += multimedia-private
CONFIG += no_private_qt_headers_warning

PLUGIN_TYPE=mediaservice
DESTDIR = ../$${PLUGIN_TYPE}

load(qt_plugin)

HEADERS += ../mockservice.h
SOURCES += mockserviceplugin3.cpp
OTHER_FILES += mockserviceplugin3.json

target.path += $$[QT_INSTALL_TESTS]/tst_qmediaserviceprovider/$${PLUGIN_TYPE}
INSTALLS += target
