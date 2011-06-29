######################################################################
#
# Mobility API project - multimedia 
#
######################################################################

TEMPLATE = subdirs

SUBDIRS += m3u

win32 {
    SUBDIRS += audiocapture
}

win32:!wince* {
    contains(directshow_enabled, yes): SUBDIRS += directshow
}

simulator: SUBDIRS += simulator

unix:!mac:!symbian {
    TMP_GST_LIBS = \
        gstreamer-0.10 >= 0.10.19 \
        gstreamer-base-0.10 >= 0.10.19 \
        gstreamer-interfaces-0.10 >= 0.10.19 \
        gstreamer-audio-0.10 >= 0.10.19 \
        gstreamer-video-0.10 >= 0.10.19

    system(pkg-config --exists \'$${TMP_GST_LIBS}\' --print-errors): {
        SUBDIRS += gstreamer
    } else {
        SUBDIRS += audiocapture
    }

    !maemo*:SUBDIRS += v4l

    contains(pulseaudio_enabled, yes) {
        SUBDIRS += pulseaudio
    }
}

mac:!simulator {
    SUBDIRS += audiocapture qt7
}

symbian:SUBDIRS += symbian
