# Generated from multimedia.pro.

#####################################################################
## declarative_multimedia Plugin:
#####################################################################

qt_internal_add_qml_module(declarative_multimedia
    URI "QtMultimedia"
    VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}" # special case
    CLASSNAME QMultimediaDeclarativeModule
    SOURCES
        multimedia.cpp
        qdeclarativecamera.cpp qdeclarativecamera_p.h
        qdeclarativecameracapture.cpp qdeclarativecameracapture_p.h
        qdeclarativecameraexposure.cpp qdeclarativecameraexposure_p.h
        qdeclarativecameraflash.cpp qdeclarativecameraflash_p.h
        qdeclarativecamerafocus.cpp qdeclarativecamerafocus_p.h
        qdeclarativecameraimageprocessing.cpp qdeclarativecameraimageprocessing_p.h
        qdeclarativecamerapreviewprovider.cpp qdeclarativecamerapreviewprovider_p.h
        qdeclarativecamerarecorder.cpp qdeclarativecamerarecorder_p.h
        qdeclarativemultimediaglobal.cpp qdeclarativemultimediaglobal_p.h
        qdeclarativeplaylist.cpp qdeclarativeplaylist_p.h
        qdeclarativetorch.cpp qdeclarativetorch_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        Qt::Network
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:multimedia.pro:<TRUE>:
# CXX_MODULE = "multimedia"
# QML_FILES = "Video.qml"
# QML_IMPORT_VERSION = "5.$$QT_MINOR_VERSION"
# TARGETPATH = "QtMultimedia"

set(qml_files
    "Video.qml"
)
set_source_files_properties(Video.qml PROPERTIES
    QT_QML_SOURCE_VERSION "5.0"
)

qt6_target_qml_files(declarative_multimedia
    FILES
        ${qml_files}
)
