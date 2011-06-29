######################################################################
#
# Mobility API project - Symbian Camera backend
#
######################################################################

TEMPLATE =      lib
CONFIG +=       plugin

TARGET =        $$qtLibraryTarget(qtmultimediakit_ecamengine)
PLUGIN_TYPE =   mediaservice
include (../../../../common.pri)

CONFIG +=       mobility
MOBILITY +=     multimedia

# Include here so that all defines are added here also
include(camera_s60.pri)

DEPENDPATH += .

INCLUDEPATH += . \
    $${SOURCE_DIR}/include \
    $${SOURCE_DIR}/src/multimedia \
    $${SOURCE_DIR}/src/multimedia/audio \
    $${SOURCE_DIR}/src/multimedia/video \
    $${SOURCE_DIR}

HEADERS += s60cameraserviceplugin.h
SOURCES += s60cameraserviceplugin.cpp

load(data_caging_paths)
TARGET.EPOCALLOWDLLDATA =   1
TARGET.UID3 =               0x2002BFC2
TARGET.CAPABILITY =         ALL -TCB

# Make a sis package from plugin + api + stub (plugin)
pluginDep.sources = $${TARGET}.dll
pluginDep.path =    $${QT_PLUGINS_BASE_DIR}/$${PLUGIN_TYPE}
DEPLOYMENT +=       pluginDep
