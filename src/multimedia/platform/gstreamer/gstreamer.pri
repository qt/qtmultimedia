DEFINES += GLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26

QMAKE_USE_PRIVATE += gstreamer gstreamer_app

SOURCES += \
    $$PWD/qgstreamerintegration.cpp \
    $$PWD/qgstreamerdevicemanager.cpp \

HEADERS += \
    $$PWD/qgstreamerintegration_p.h \
    $$PWD/qgstreamerdevicemanager_p.h \

include(audio/audio.pri)
include(common/common.pri)
use_camerabin {
    include(camerabin/camerabin.pri)
    DEFINES += GST_USE_CAMERABIN
} else {
    include(mediacapture/mediacapture.pri)
}
include(mediaplayer/mediaplayer.pri)

qtConfig(gstreamer_gl): QMAKE_USE += gstreamer_gl

android {
    LIBS_PRIVATE += \
        -L$$(GSTREAMER_ROOT_ANDROID)/armv7/lib \
        -Wl,--_p.hole-archive \
        -lgstapp-1.0 -lgstreamer-1.0 -lgstaudio-1.0 -lgsttag-1.0 -lgstvideo-1.0 -lgstbase-1.0 -lgstpbutils-1.0 \
        -lgobject-2.0 -lgmodule-2.0 -lglib-2.0 -lffi -lintl -liconv -lorc-0.4 \
        -Wl,--no-_p.hole-archive
}

