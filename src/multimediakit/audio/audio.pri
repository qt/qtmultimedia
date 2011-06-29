INCLUDEPATH += audio

PUBLIC_HEADERS += audio/qaudio.h \
           audio/qaudioformat.h \
           audio/qaudioinput.h \
           audio/qaudiooutput.h \
           audio/qaudiodeviceinfo.h \
           audio/qaudiosystemplugin.h \
           audio/qaudiosystem.h 

PRIVATE_HEADERS += audio/qaudiodevicefactory_p.h audio/qaudiopluginloader_p.h


SOURCES += audio/qaudio.cpp \
           audio/qaudioformat.cpp  \
           audio/qaudiodeviceinfo.cpp \
           audio/qaudiooutput.cpp \
           audio/qaudioinput.cpp \
           audio/qaudiosystemplugin.cpp \
           audio/qaudiosystem.cpp \
           audio/qaudiodevicefactory.cpp \
           audio/qaudiopluginloader.cpp

mac {
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
    !wince*:LIBS += -lwinmm
    wince*:LIBS += -lcoredll
    LIBS += -lstrmiids -lole32 -loleaut32
}

symbian {
    INCLUDEPATH += $${EPOCROOT}epoc32/include/mmf/common
    INCLUDEPATH += $${EPOCROOT}epoc32/include/mmf/server

    PRIVATE_HEADERS += audio/qaudio_symbian_p.h \
               audio/qaudiodeviceinfo_symbian_p.h \
               audio/qaudioinput_symbian_p.h \
               audio/qaudiooutput_symbian_p.h

    SOURCES += audio/qaudio_symbian_p.cpp \
               audio/qaudiodeviceinfo_symbian_p.cpp \
               audio/qaudioinput_symbian_p.cpp \
               audio/qaudiooutput_symbian_p.cpp

    LIBS += -lmmfdevsound
}

unix:!mac:!symbian {
    contains(pulseaudio_enabled, yes) {
        DEFINES += QT_NO_AUDIO_BACKEND
    }
    else:contains(QT_CONFIG, alsa) {
        linux-*|freebsd-*|openbsd-* {
            DEFINES += HAS_ALSA
            PRIVATE_HEADERS += audio/qaudiooutput_alsa_p.h audio/qaudioinput_alsa_p.h audio/qaudiodeviceinfo_alsa_p.h
            SOURCES += audio/qaudiodeviceinfo_alsa_p.cpp \
                   audio/qaudiooutput_alsa_p.cpp \
                   audio/qaudioinput_alsa_p.cpp
            LIBS_PRIVATE += -lasound
        }
    }
}
