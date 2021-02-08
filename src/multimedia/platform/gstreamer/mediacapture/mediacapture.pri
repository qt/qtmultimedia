INCLUDEPATH += $$PWD

HEADERS += $$PWD/qgstreamercaptureservice_p.h \
    $$PWD/qgstreamercapturesession_p.h \
    $$PWD/qgstreamerrecordercontrol_p.h \
    $$PWD/qgstreamercameracontrol_p.h \
    $$PWD/qgstreamercapturemetadatacontrol_p.h \
    $$PWD/qgstreamerimagecapturecontrol_p.h \
    $$PWD/qgstreamervideoinput_p.h

SOURCES += $$PWD/qgstreamercaptureservice.cpp \
    $$PWD/qgstreamercapturesession.cpp \
    $$PWD/qgstreamerrecordercontrol.cpp \
    $$PWD/qgstreamercameracontrol.cpp \
    $$PWD/qgstreamercapturemetadatacontrol.cpp \
    $$PWD/qgstreamerimagecapturecontrol.cpp \
    $$PWD/qgstreamervideoinput.cpp

# Camera usage with gstreamer needs to have
CONFIG += use_gstreamer_camera

