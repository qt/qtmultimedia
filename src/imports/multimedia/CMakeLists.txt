# Generated from multimedia.pro.

#####################################################################
## declarative_multimedia Plugin:
#####################################################################

qt_internal_add_qml_module(quickmultimedia
    URI "QtMultimedia"
    VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}" # special case
    CLASSNAME QMultimediaQuickModule
    SOURCES
        multimedia.cpp
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

qt6_target_qml_files(quickmultimedia
    FILES
        ${qml_files}
)
