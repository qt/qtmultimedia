SOURCES = pulseaudio.cpp
CONFIG -= qt
LIBS +=
CONFIG += link_pkgconfig

PKGCONFIG += \
        libpulse \
        libpulse-mainloop-glib
