android|ios: error("This is a commandline tool that is not supported on mobile platforms")

TEMPLATE = app
TARGET = devices

SOURCES = main.cpp

QT += multimedia
CONFIG += console

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/devices
INSTALLS += target
