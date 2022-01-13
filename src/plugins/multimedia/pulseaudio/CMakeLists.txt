
qt_internal_add_plugin(QPulseAudioMediaPlugin
    OUTPUT_NAME pulseaudiomediaplugin
    PLUGIN_TYPE multimedia
    SOURCES
        qpulseaudiodevice.cpp qpulseaudiodevice_p.h
        qaudioengine_pulse.cpp qaudioengine_pulse_p.h
        qpulseaudiosource.cpp qpulseaudiosource_p.h
        qpulseaudiosink.cpp qpulseaudiosink_p.h
        qpulsehelpers.cpp qpulsehelpers_p.h
        qpulseaudiomediadevices.cpp qpulseaudiomediadevices_p.h
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
        WrapPulseAudio::WrapPulseAudio
)
