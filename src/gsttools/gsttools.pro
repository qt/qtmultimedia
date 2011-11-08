TEMPLATE = lib

TARGET = qgsttools_p
QPRO_PWD = $$PWD
QT = core multimedia-private

!static:DEFINES += QT_MAKEDLL

unix:!maemo*:contains(QT_CONFIG, alsa) {
DEFINES += HAVE_ALSA
LIBS += \
    -lasound
}

CONFIG += link_pkgconfig

PKGCONFIG += \
    gstreamer-0.10 \
    gstreamer-base-0.10 \
    gstreamer-interfaces-0.10 \
    gstreamer-audio-0.10 \
    gstreamer-video-0.10 \
    gstreamer-pbutils-0.10

maemo*:PKGCONFIG +=gstreamer-plugins-bad-0.10
contains(config_test_gstreamer_appsrc, yes): PKGCONFIG += gstreamer-app-0.10

contains(config_test_resourcepolicy, yes) {
    DEFINES += HAVE_RESOURCE_POLICY
    PKGCONFIG += libresourceqt1
}

# Header files must go inside source directory of a module
# to be installed by syncqt.
INCLUDEPATH += ../multimedia/gsttools_headers/
DEPENDPATH += ../multimedia/gsttools_headers/

PRIVATE_HEADERS += \
    qgstbufferpoolinterface_p.h \
    qgstreamerbushelper_p.h \
    qgstreamermessage_p.h \
    qgstutils_p.h \
    qgstvideobuffer_p.h \
    qvideosurfacegstsink_p.h \


SOURCES += \
    qgstbufferpoolinterface.cpp \
    qgstreamerbushelper.cpp \
    qgstreamermessage.cpp \
    qgstutils.cpp \
    qgstvideobuffer.cpp \
    qvideosurfacegstsink.cpp \

!win32:!contains(QT_CONFIG,embedded):!mac:!simulator:!contains(QT_CONFIG, qpa) {
    LIBS += -lXv -lX11 -lXext

    PRIVATE_HEADERS += \
        qgstxvimagebuffer_p.h \


    SOURCES += \
        qgstxvimagebuffer.cpp \
}

HEADERS += $$PRIVATE_HEADERS

DESTDIR = $$QT.multimedia.libs
target.path = $$[QT_INSTALL_LIBS]

INSTALLS += target
