######################################################################
#
# QtMultimedia
#
######################################################################

TEMPLATE = subdirs

SUBDIRS += m3u

win32 {
    SUBDIRS += audiocapture
}

win32 {
    contains(config_test_directshow, yes): SUBDIRS += directshow
    contains(config_test_wmf, yes) : SUBDIRS += wmf
}

simulator: SUBDIRS += simulator

unix:!mac {
    contains(config_test_gstreamer, yes) {
       SUBDIRS += gstreamer
    } else {
        SUBDIRS += audiocapture
    }

    # v4l is turned off because it is not supported in Qt 5
    # !maemo*:SUBDIRS += v4l

    contains(config_test_pulseaudio, yes) {
        SUBDIRS += pulseaudio
    }
}

mac:!simulator {
    SUBDIRS += audiocapture

    # Currently we need qmacdefines_mac.h from QtWidgets, so this depends on that :/
    # despite the code not really using them.
    contains(config_test_widgets, yes):SUBDIRS += qt7
}

