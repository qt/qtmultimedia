INCLUDEPATH += audio

PUBLIC_HEADERS += \
           audio/qaudio.h \
           audio/qaudiobuffer.h \
           audio/qaudioformat.h \
           audio/qaudioinput.h \
           audio/qaudiooutput.h \
           audio/qaudiodeviceinfo.h \
           audio/qaudiosystemplugin.h \
           audio/qaudiosystem.h  \
           audio/qsoundeffect.h \
           audio/qsound.h \
           audio/qaudioprobe.h \
           audio/qaudiodecoder.h

PRIVATE_HEADERS += \
           audio/qaudiobuffer_p.h \
           audio/qaudiodevicefactory_p.h \
           audio/qwavedecoder_p.h \
           audio/qsamplecache_p.h \
           audio/qaudiohelpers_p.h

SOURCES += \
           audio/qaudio.cpp \
           audio/qaudioformat.cpp  \
           audio/qaudiodeviceinfo.cpp \
           audio/qaudiooutput.cpp \
           audio/qaudioinput.cpp \
           audio/qaudiosystemplugin.cpp \
           audio/qaudiosystem.cpp \
           audio/qaudiodevicefactory.cpp \
           audio/qsoundeffect.cpp \
           audio/qwavedecoder_p.cpp \
           audio/qsamplecache_p.cpp \
           audio/qsound.cpp \
           audio/qaudiobuffer.cpp \
           audio/qaudioprobe.cpp \
           audio/qaudiodecoder.cpp \
           audio/qaudiohelpers.cpp

mac:!ios {

    PRIVATE_HEADERS +=  audio/qaudioinput_mac_p.h \
                audio/qaudiooutput_mac_p.h \
                audio/qaudiodeviceinfo_mac_p.h \
                audio/qaudio_mac_p.h

    SOURCES += audio/qaudiodeviceinfo_mac_p.cpp \
               audio/qaudiooutput_mac_p.cpp \
               audio/qaudioinput_mac_p.cpp \
               audio/qaudio_mac.cpp
    LIBS += -framework ApplicationServices -framework CoreAudio -framework AudioUnit -framework AudioToolbox
}

win32 {
    PRIVATE_HEADERS += audio/qaudioinput_win32_p.h audio/qaudiooutput_win32_p.h audio/qaudiodeviceinfo_win32_p.h
    SOURCES += audio/qaudiodeviceinfo_win32_p.cpp \
               audio/qaudiooutput_win32_p.cpp \
               audio/qaudioinput_win32_p.cpp
    LIBS += -lwinmm -lstrmiids -lole32 -loleaut32
}

unix:!mac {
    config_pulseaudio {
        DEFINES += QT_NO_AUDIO_BACKEND
        CONFIG += link_pkgconfig
        PKGCONFIG += libpulse

        DEFINES += QT_MULTIMEDIA_PULSEAUDIO
        PRIVATE_HEADERS += audio/qsoundeffect_pulse_p.h
        SOURCES += audio/qsoundeffect_pulse_p.cpp
        !maemo*:DEFINES += QTM_PULSEAUDIO_DEFAULTBUFFER
    } else {
        DEFINES += QT_MULTIMEDIA_QAUDIO
        PRIVATE_HEADERS += audio/qsoundeffect_qaudio_p.h
        SOURCES += audio/qsoundeffect_qaudio_p.cpp

        config_alsa {
            DEFINES += HAS_ALSA
            PRIVATE_HEADERS += audio/qaudiooutput_alsa_p.h audio/qaudioinput_alsa_p.h audio/qaudiodeviceinfo_alsa_p.h
            SOURCES += audio/qaudiodeviceinfo_alsa_p.cpp \
                audio/qaudiooutput_alsa_p.cpp \
                audio/qaudioinput_alsa_p.cpp
            LIBS_PRIVATE += -lasound
        }
    }
} else {
    DEFINES += QT_MULTIMEDIA_QAUDIO
    PRIVATE_HEADERS += audio/qsoundeffect_qaudio_p.h
    SOURCES += audio/qsoundeffect_qaudio_p.cpp
}
