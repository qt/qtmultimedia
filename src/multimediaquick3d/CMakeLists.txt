#####################################################################
## Quick3D.Sound Module:
#####################################################################

qt_internal_add_qml_module(Quick3DSpatialAudioPrivate
    URI "QtQuick3D.SpatialAudio"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QQuick3DSpatialAudioModule
    PLUGIN_TARGET quick3dspatialaudio
    NO_GENERATE_PLUGIN_SOURCE
    NO_PLUGIN_OPTIONAL
    DEPENDENCIES QtQuick QtQuick3DPrivate QtMultimedia
    CONFIG_MODULE_NAME quick3dspatialaudio
    INTERNAL_MODULE
    SOURCES
        qquick3dspatialaudiolistener.cpp qquick3dspatialaudiolistener_p.h
        qquick3dspatialaudiosoundsource.cpp qquick3dspatialaudiosoundsource_p.h
        qquick3dspatialaudiostereosource.cpp qquick3dspatialaudiostereosource_p.h
        qquick3dspatialaudioengine.cpp qquick3dspatialaudioengine_p.h
        qtquick3dsoundglobal_p.h
        qtquick3dsoundtypes_p.h
    QML_FILES
    PUBLIC_LIBRARIES
        Qt::Quick3DPrivate
        Qt::Multimedia
    GENERATE_CPP_EXPORTS
)

target_sources(quick3dspatialaudio PRIVATE quick3dspatialaudio_plugin.cpp)
