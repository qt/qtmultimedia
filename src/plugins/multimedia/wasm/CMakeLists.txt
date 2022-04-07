qt_internal_add_plugin(QWasmMediaPlugin
    OUTPUT_NAME wasmmediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qwasmmediaintegration.cpp qwasmmediaintegration_p.h
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        openal
)

