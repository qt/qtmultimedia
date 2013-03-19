######################################################################
#
# Qt Multimedia
#
######################################################################

TEMPLATE = subdirs

SUBDIRS += m3u

android {
   SUBDIRS += android
}

blackberry {
    SUBDIRS += blackberry
}

qnx {
    SUBDIRS += qnx
}

win32 {
    SUBDIRS += audiocapture
}

win32 {
    config_directshow: SUBDIRS += directshow
    config_wmf: SUBDIRS += wmf
}

unix:!mac {
    config_gstreamer {
       SUBDIRS += gstreamer
    } else {
        SUBDIRS += audiocapture
    }

    # v4l is turned off because it is not supported in Qt 5
    # !maemo*:SUBDIRS += v4l

    config_pulseaudio {
        SUBDIRS += pulseaudio
    }
}

mac:!simulator {
    SUBDIRS += audiocapture

    !ios {
        SUBDIRS += qt7
        config_avfoundation: SUBDIRS += avfoundation
    }
}

