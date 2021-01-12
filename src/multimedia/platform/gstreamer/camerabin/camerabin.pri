
HEADERS += \
    $$PWD/camerabinserviceplugin_p.h \
    $$PWD/camerabinservice_p.h \
    $$PWD/camerabinsession_p.h \
    $$PWD/camerabincontrol_p.h \
    $$PWD/camerabinaudioencoder_p.h \
    $$PWD/camerabinimageencoder_p.h \
    $$PWD/camerabinrecorder_p.h \
    $$PWD/camerabincontainer_p.h \
    $$PWD/camerabinimagecapture_p.h \
    $$PWD/camerabinimageprocessing_p.h \
    $$PWD/camerabinmetadata_p.h \
    $$PWD/camerabinvideoencoder_p.h \

SOURCES += \
    $$PWD/camerabinserviceplugin.cpp \
    $$PWD/camerabinservice.cpp \
    $$PWD/camerabinsession.cpp \
    $$PWD/camerabincontrol.cpp \
    $$PWD/camerabinaudioencoder.cpp \
    $$PWD/camerabincontainer.cpp \
    $$PWD/camerabinimagecapture.cpp \
    $$PWD/camerabinimageencoder.cpp \
    $$PWD/camerabinimageprocessing.cpp \
    $$PWD/camerabinmetadata.cpp \
    $$PWD/camerabinrecorder.cpp \
    $$PWD/camerabinvideoencoder.cpp \

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
