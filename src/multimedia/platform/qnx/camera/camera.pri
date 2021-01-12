INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/bbcameraaudioencodersettingscontrol_p.h \
    $$PWD/bbcameracontrol_p.h \
    $$PWD/bbcameraexposurecontrol_p.h \
    $$PWD/bbcamerafocuscontrol_p.h \
    $$PWD/bbcameraimagecapturecontrol_p.h \
    $$PWD/bbcameraimageprocessingcontrol_p.h \
    $$PWD/bbcameramediarecordercontrol_p.h \
    $$PWD/bbcameraorientatio_p.handler.h \
    $$PWD/bbcameraservice_p.h \
    $$PWD/bbcamerasession_p.h \
    $$PWD/bbcameravideoencodersettingscontrol_p.h \
    $$PWD/bbcameraviewfindersettingscontrol_p.h \
    $$PWD/bbimageencodercontrol_p.h \
    $$PWD/bbmediastoragelocation_p.h \
    $$PWD/bbvideodeviceselectorcontrol_p.h \
    $$PWD/bbvideorenderercontrol_p.h

SOURCES += \
    $$PWD/bbcameraaudioencodersettingscontrol.cpp \
    $$PWD/bbcameracontrol.cpp \
    $$PWD/bbcameraexposurecontrol.cpp \
    $$PWD/bbcamerafocuscontrol.cpp \
    $$PWD/bbcameraimagecapturecontrol.cpp \
    $$PWD/bbcameraimageprocessingcontrol.cpp \
    $$PWD/bbcameramediarecordercontrol.cpp \
    $$PWD/bbcameraorientatio_p.handler.cpp \
    $$PWD/bbcameraservice.cpp \
    $$PWD/bbcamerasession.cpp \
    $$PWD/bbcameravideoencodersettingscontrol.cpp \
    $$PWD/bbcameraviewfindersettingscontrol.cpp \
    $$PWD/bbimageencodercontrol.cpp \
    $$PWD/bbmediastoragelocation.cpp \
    $$PWD/bbvideodeviceselectorcontrol.cpp \
    $$PWD/bbvideorenderercontrol.cpp

LIBS += -lcamapi -laudio_manager

