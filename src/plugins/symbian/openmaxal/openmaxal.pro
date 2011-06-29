TEMPLATE = lib

CONFIG += plugin
TARGET = $$qtLibraryTarget(qtmultimediakit_openmaxalengine)
PLUGIN_TYPE = mediaservice
include (../../../../common.pri)
qtAddLibrary(QtMultimediaKit)

#includes here so that all defines are added here also
include(mediaplayer/mediaplayer.pri)
include(mediarecorder/mediarecorder.pri)
include(radiotuner/radiotuner.pri)

DEPENDPATH += .

HEADERS += qxamediaserviceproviderplugin.h \
           qxacommon.h \
           xacommon.h

SOURCES += qxamediaserviceproviderplugin.cpp

# Input parameters for the generated bld.inf file
# -----------------------------------------------
SYMBIAN_PLATFORMS = DEFAULT

# Input parameters for the generated mmp file
# -------------------------------------------
load(data_caging_paths)
TARGET.UID3 = 0x10207CA1
TARGET.CAPABILITY = ALL -TCB
TARGET.EPOCALLOWDLLDATA = 1
MMP_RULES += EXPORTUNFROZEN

# Macros controlling debug traces
#DEFINES += PROFILE_TIME
#DEFINES += PROFILE_RAM_USAGE
#DEFINES += PROFILE_HEAP_USAGE
#DEFINES += PLUGIN_QT_TRACE_ENABLED
#DEFINES += PLUGIN_QT_SIGNAL_EMIT_TRACE_ENABLED
#DEFINES += PLUGIN_SYMBIAN_TRACE_ENABLED

INCLUDEPATH += $$MW_LAYER_SYSTEMINCLUDE
INCLUDEPATH += /epoc32/include/platform/mw/khronos


# Input parameters for qmake to make the dll a qt plugin
pluginDep.sources = $${TARGET}.dll
pluginDep.path = $${QT_PLUGINS_BASE_DIR}/$${PLUGIN_TYPE}
DEPLOYMENT += pluginDep      

LIBS += \
    -lQtMultimediaKit \
    -lopenmaxal

# check for PROFILE_RAM_USAGE
contains(DEFINES, PROFILE_RAM_USAGE) {
    LIBS += -lhal
}
