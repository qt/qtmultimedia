INCLUDEPATH += effects

unix:!mac {
   contains(config_test_pulseaudio, yes) {
        CONFIG += link_pkgconfig
        PKGCONFIG += libpulse

        DEFINES += QT_MULTIMEDIA_PULSEAUDIO
        PRIVATE_HEADERS += effects/qsoundeffect_pulse_p.h
        SOURCES += effects/qsoundeffect_pulse_p.cpp
        !maemo*:DEFINES += QTM_PULSEAUDIO_DEFAULTBUFFER
    } else {
        DEFINES += QT_MULTIMEDIA_QMEDIAPLAYER
        PRIVATE_HEADERS += effects/qsoundeffect_qmedia_p.h
        SOURCES += effects/qsoundeffect_qmedia_p.cpp
    }
} else:!qpa {
    PRIVATE_HEADERS += effects/qsoundeffect_qsound_p.h
    SOURCES += effects/qsoundeffect_qsound_p.cpp
} else {
    DEFINES += QT_MULTIMEDIA_QMEDIAPLAYER
    PRIVATE_HEADERS += effects/qsoundeffect_qmedia_p.h
    SOURCES += effects/qsoundeffect_qmedia_p.cpp
}

PRIVATE_HEADERS += \
        effects/qsoundeffect_p.h \
        effects/qwavedecoder_p.h \
        effects/qsamplecache_p.h

SOURCES += \
    effects/qsoundeffect.cpp \
    effects/qwavedecoder_p.cpp \
    effects/qsamplecache_p.cpp

HEADERS +=
