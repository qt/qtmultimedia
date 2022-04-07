
qt_internal_extend_target(Multimedia CONDITION QT_FEATURE_alsa
    SOURCES
        qalsaaudiodevice.cpp qalsaaudiodevice_p.h
        qalsaaudiosource.cpp qalsaaudiosource_p.h
        qalsaaudiosink.cpp qalsaaudiosink_p.h
        qalsamediadevices.cpp qalsamediadevices_p.h
        qalsaintegration.cpp qalsaintegration_p.h
    LIBRARIES
        ALSA::ALSA
)
