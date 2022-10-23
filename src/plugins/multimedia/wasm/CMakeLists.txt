qt_internal_add_plugin(QWasmMediaPlugin
    OUTPUT_NAME wasmmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qwasmmediaintegration.cpp qwasmmediaintegration_p.h
        mediaplayer/qwasmmediaplayer.cpp mediaplayer/qwasmmediaplayer_p.h
        mediaplayer/qwasmvideosink.cpp mediaplayer/qwasmvideosink_p.h
        common/qwasmvideooutput.cpp common/qwasmvideooutput_p.h
        common/qwasmaudiooutput.cpp common/qwasmaudiooutput_p.h
        common/qwasmaudioinput.cpp common/qwasmaudioinput_p.h

    INCLUDE_DIRECTORIES
        common
        mediaplayer
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        openal
)

