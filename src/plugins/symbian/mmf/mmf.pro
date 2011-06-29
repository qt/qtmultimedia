TEMPLATE = lib

CONFIG += plugin
TARGET = $$qtLibraryTarget(qtmultimediakit_mmfengine)
PLUGIN_TYPE = mediaservice
include (../../../../common.pri)
qtAddLibrary(QtMultimediaKit)

#includes here so that all defines are added here also
include(mediaplayer/mediaplayer_s60.pri)
include(radio/radio.pri)

QT += network

# we include mmf audiorecording only if we are not building openmaxal based backend
!contains(openmaxal_symbian_enabled, yes) {
    message("Enabling mmf mediarecording backend")
    include(audiosource/audiosource_s60.pri)
}

DEPENDPATH += .
INCLUDEPATH += . \
    $${SOURCE_DIR}/include \
    $${SOURCE_DIR}/src/multimedia \
    $${SOURCE_DIR}/src/multimedia/audio \
    $${SOURCE_DIR}/src/multimedia/video \
    $${SOURCE_DIR}/plugins/multimedia/symbian/mmf/inc \
    $${SOURCE_DIR}


HEADERS += s60mediaserviceplugin.h \
    s60formatsupported.h

SOURCES += s60mediaserviceplugin.cpp \
    s60formatsupported.cpp

contains(S60_VERSION, 3.2)|contains(S60_VERSION, 3.1) {
    DEFINES += PRE_S60_50_PLATFORM
}
contains(mmf_http_cookies_enabled, yes) {
    DEFINES += HTTP_COOKIES_ENABLED
}
load(data_caging_paths)
TARGET.EPOCALLOWDLLDATA = 1
TARGET.UID3=0x2002AC76
TARGET.CAPABILITY = ALL -TCB
MMP_RULES += EXPORTUNFROZEN

#make a sis package from plugin + api + stub (plugin)
pluginDep.sources = $${TARGET}.dll
pluginDep.path = $${QT_PLUGINS_BASE_DIR}/$${PLUGIN_TYPE}
DEPLOYMENT += pluginDep

#Media API spesific deployment
QtMediaDeployment.sources = QtMultimediaKit.dll
QtMediaDeployment.path = /sys/bin

DEPLOYMENT += QtMediaDeployment
