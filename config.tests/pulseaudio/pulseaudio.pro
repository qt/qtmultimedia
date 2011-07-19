CONFIG -= qt
LIBS +=
CONFIG += link_pkgconfig

requires(unix)

PKGCONFIG += \
        libpulse \
        libpulse-mainloop-glib

SOURCES = pulseaudio.cpp
