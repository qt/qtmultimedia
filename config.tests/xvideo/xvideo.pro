TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

requires(unix)

# Input
SOURCES += main.cpp

CONFIG += link_pkgconfig

PKGCONFIG += \
    x11 \
    xext \
    xv

