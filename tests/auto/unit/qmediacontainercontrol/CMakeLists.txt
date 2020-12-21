# Generated from qmediacontainercontrol.pro.

#####################################################################
## tst_qmediacontainercontrol Test:
#####################################################################

qt_add_test(tst_qmediacontainercontrol
    SOURCES
        ../qmultimedia_common/mockmediacontainercontrol.h
        ../qmultimedia_common/mockmetadatareadercontrol.h
        ../qmultimedia_common/mockmetadatawritercontrol.h
        tst_qmediacontainercontrol.cpp
    INCLUDE_DIRECTORIES
        ../../../src/multimedia
        ../qmultimedia_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)
