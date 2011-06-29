INCLUDEPATH += $$PWD

message("VideoOutput: using common implementation")

contains(surfaces_s60_enabled, yes) {
    message("VideoOutput: graphics surface rendering supported")
    DEFINES += VIDEOOUTPUT_GRAPHICS_SURFACES
} else {
    message("VideoOutput: no graphics surface rendering support - DSA only")
}

exists($$[QT_INSTALL_HEADERS]/QtGui/private/qwidget_p.h) {
    DEFINES += PRIVATE_QTGUI_HEADERS_AVAILABLE
    message("VideoOutput: private QtGui headers are available")
} else {
    message("VideoOutput: private QtGui headers not available - video and viewfinder may not be rendered correctly")
}

HEADERS += $$PWD/s60videodisplay.h         \
           $$PWD/s60videooutpututils.h     \
           $$PWD/s60videowidget.h          \
           $$PWD/s60videowidgetcontrol.h   \
           $$PWD/s60videowidgetdisplay.h   \
           $$PWD/s60videowindowcontrol.h   \
           $$PWD/s60videowindowdisplay.h

SOURCES += $$PWD/s60videodisplay.cpp       \
           $$PWD/s60videooutpututils.cpp   \
           $$PWD/s60videowidget.cpp        \
           $$PWD/s60videowidgetcontrol.cpp \
           $$PWD/s60videowidgetdisplay.cpp \
           $$PWD/s60videowindowcontrol.cpp \
           $$PWD/s60videowindowdisplay.cpp

LIBS *= -lcone
LIBS *= -lws32

