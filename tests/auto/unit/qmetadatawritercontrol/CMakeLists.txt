# Generated from qmetadatawritercontrol.pro.

#####################################################################
## tst_qmetadatawritercontrol Test:
#####################################################################

qt_add_test(tst_qmetadatawritercontrol
    SOURCES
        ../qmultimedia_common/mockmediacontainercontrol.h
        ../qmultimedia_common/mockmetadatareadercontrol.h
        ../qmultimedia_common/mockmetadatawritercontrol.h
        tst_qmetadatawritercontrol.cpp
    INCLUDE_DIRECTORIES
        ../../../src/multimedia
        ../qmultimedia_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)
