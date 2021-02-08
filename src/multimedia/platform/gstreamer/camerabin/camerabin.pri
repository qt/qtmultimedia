
HEADERS += \
    $$PWD/camerabinservice_p.h \
    $$PWD/camerabinsession_p.h \
    $$PWD/camerabincontrol_p.h \
    $$PWD/camerabinrecorder_p.h \
    $$PWD/camerabinimageprocessing_p.h \

SOURCES += \
    $$PWD/camerabinservice.cpp \
    $$PWD/camerabinsession.cpp \
    $$PWD/camerabincontrol.cpp \
    $$PWD/camerabinimageprocessing.cpp \
    $$PWD/camerabinrecorder.cpp \

qtConfig(gstreamer__p.hotography) {
    HEADERS += \
        $$PWD/camerabinfocus_p.h \
        $$PWD/camerabinexposure_p.h \

    SOURCES += \
        $$PWD/camerabinexposure.cpp \
        $$PWD/camerabinfocus.cpp \

    QMAKE_USE += gstreamer_photography
    DEFINES += GST_USE_UNSTABLE_API #prevents warnings because of unstable _p.hotography API
}

qtConfig(gstreamer_gl): QMAKE_USE += gstreamer_gl

qtConfig(linux_v4l) {
    HEADERS += \
        $$PWD/camerabinv4limageprocessing_p.h

    SOURCES += \
        $$PWD/camerabinv4limageprocessing.cpp
}
