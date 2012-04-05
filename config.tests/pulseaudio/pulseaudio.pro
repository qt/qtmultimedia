CONFIG -= qt
LIBS +=
CONFIG += link_pkgconfig

PKGCONFIG += \
        libpulse \
        libpulse-mainloop-glib

SOURCES = pulseaudio.cpp
