# Generated from qmetadatareadercontrol.pro.

#####################################################################
## tst_qmetadatareadercontrol Test:
#####################################################################

qt_add_test(tst_qmetadatareadercontrol
    SOURCES
        ../qmultimedia_common/mockmediacontainercontrol.h
        ../qmultimedia_common/mockmetadatareadercontrol.h
        ../qmultimedia_common/mockmetadatawritercontrol.h
        tst_qmetadatareadercontrol.cpp
    INCLUDE_DIRECTORIES
        ../../../src/multimedia
        ../qmultimedia_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)
