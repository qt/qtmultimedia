
load(qt_module)

TARGET = mockserviceplugin1
QT += multimedia-private
CONFIG += no_private_qt_headers_warning

PLUGIN_TYPE=mediaservice
DESTDIR = ../$${PLUGIN_TYPE}

load(qt_plugin)

HEADERS += ../mockservice.h
SOURCES += mockserviceplugin1.cpp
OTHER_FILES += mockserviceplugin1.json

target.path += $$[QT_INSTALL_TESTS]/tst_qmediaserviceprovider/$${PLUGIN_TYPE}
INSTALLS += target
