qt_internal_add_plugin(QWasmMediaPlugin
    OUTPUT_NAME wasmmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qwasmmediaintegration.cpp qwasmmediaintegration_p.h
        qwasmmediadevices.cpp qwasmmediadevices_p.h
        audio/qwasmaudiosource.cpp audio/qwasmaudiosource_p.h
        audio/qwasmaudiosink.cpp audio/qwasmaudiosink_p.h
        audio/qwasmaudiodevice.cpp audio/qwasmaudiodevice_p.h
    INCLUDE_DIRECTORIES
        audio
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        openal
)

