INCLUDEPATH += $$PWD

maemo5 {
    QT += dbus

    CONFIG += link_pkgconfig

    PKGCONFIG += gstreamer-0.10

    LIBS += -lasound

    HEADERS += \
	$$PWD/v4lradiocontrol_maemo5.h \
	$$PWD/v4lradioservice.h

    SOURCES += \
	$$PWD/v4lradiocontrol_maemo5.cpp \
	$$PWD/v4lradioservice.cpp

} else {

HEADERS += \
    $$PWD/v4lradiocontrol.h \
    $$PWD/v4lradioservice.h

SOURCES += \
    $$PWD/v4lradiocontrol.cpp \
    $$PWD/v4lradioservice.cpp
}
