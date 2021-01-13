QT += network

win32:!qtHaveModule(opengl) {
    LIBS_PRIVATE += -lgdi32 -luser32
}

INCLUDEPATH += .

HEADERS += \
    $$PWD/wmfserviceplugin_p.h \
    $$PWD/mfstream_p.h \
    $$PWD/sourceresolver_p.h \
    $$PWD/qwindowsdevicemanager_p.h \
    $$PWD/qwindowsintegration_p.h

SOURCES += \
    $$PWD/wmfserviceplugin.cpp \
    $$PWD/mfstream.cpp \
    $$PWD/sourceresolver.cpp \
    $$PWD/qwindowsdevicemanager.cpp \
    $$PWD/qwindowsintegration.cpp

include (evr/evr.pri)
include (player/player.pri)
include (decoder/decoder.pri)
