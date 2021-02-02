HEADERS += \
    $$PWD/avfcameradebug_p.h \
    $$PWD/avfcameracontrol_p.h \
    $$PWD/avfcamerametadatacontrol_p.h \
    $$PWD/avfimagecapturecontrol_p.h \
    $$PWD/avfcameraservice_p.h \
    $$PWD/avfcamerasession_p.h \
    $$PWD/avfstoragelocation_p.h \
    $$PWD/avfcamerarenderercontrol_p.h \
    $$PWD/avfcamerafocuscontrol_p.h \
    $$PWD/avfcameraexposurecontrol_p.h \
    $$PWD/avfcamerautility_p.h \
    $$PWD/avfimageencodercontrol_p.h \
    $$PWD/avfcamerawindowcontrol_p.h \

SOURCES += \
    $$PWD/avfcameracontrol.mm \
    $$PWD/avfcamerametadatacontrol.mm \
    $$PWD/avfimagecapturecontrol.mm \
    $$PWD/avfcameraservice.mm \
    $$PWD/avfcamerasession.mm \
    $$PWD/avfstoragelocation.mm \
    $$PWD/avfcamerarenderercontrol.mm \
    $$PWD/avfcamerafocuscontrol.mm \
    $$PWD/avfcameraexposurecontrol.mm \
    $$PWD/avfcamerautility.mm \
    $$PWD/avfimageencodercontrol.mm \
    $$PWD/avfcamerawindowcontrol.mm \

osx {

HEADERS += $$PWD/avfmediarecordercontrol_p.h
SOURCES += $$PWD/avfmediarecordercontrol.mm

}

ios {

HEADERS += \
           $$PWD/avfmediaassetwriter_p.h \
           $$PWD/avfmediarecordercontrol_ios_p.h
SOURCES += \
           $$PWD/avfmediaassetwriter.mm \
           $$PWD/avfmediarecordercontrol_ios.mm

}
