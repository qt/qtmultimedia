TEMPLATE = lib
CONFIG += plugin
TARGET = $$qtLibraryTarget(qsimulatorengine)
PLUGIN_TYPE=mediaservice

include(../../../common.pri)
INCLUDEPATH+=$${SOURCE_DIR}/src/multimedia \
             $${SOURCE_DIR}/src/multimedia/video \
             $${SOURCE_DIR}/src/multimedia/audio \
             $${SOURCE_DIR}/src/mobilitysimulator

CONFIG += mobility
MOBILITY = multimedia

DEPENDPATH += .

# Input
HEADERS += \
    qsimulatormultimediaconnection_p.h \
    qsimulatormultimediadata_p.h \
    qsimulatorserviceplugin.h

SOURCES += \
    qsimulatormultimediaconnection.cpp \
    qsimulatormultimediadata.cpp \
    qsimulatorserviceplugin.cpp \

include(camera/simulatorcamera.pri)
