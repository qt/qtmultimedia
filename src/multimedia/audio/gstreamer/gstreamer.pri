HEADERS += audio/gstreamer/qaudiointerface_gstreamer_p.h \
           audio/gstreamer/qaudiodeviceinfo_gstreamer_p.h \
           audio/gstreamer/qaudiooutput_gstreamer_p.h \
           audio/gstreamer/qaudioinput_gstreamer_p.h \
           audio/gstreamer/qaudioengine_gstreamer_p.h \

SOURCES += audio/gstreamer/qaudiointerface_gstreamer.cpp \
           audio/gstreamer/qaudiodeviceinfo_gstreamer.cpp \
           audio/gstreamer/qaudiooutput_gstreamer.cpp \
           audio/gstreamer/qaudioinput_gstreamer.cpp \
           audio/gstreamer/qaudioengine_gstreamer.cpp \

QMAKE_USE_PRIVATE += gstreamer

qtConfig(gstreamer_app): \
    QMAKE_USE += gstreamer_app

