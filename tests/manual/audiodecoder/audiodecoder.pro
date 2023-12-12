TEMPLATE = app
TARGET = audiodecoder

HEADERS = \
    audiodecoder.h
SOURCES = main.cpp \
    audiodecoder.cpp

QT += multimedia
CONFIG += console

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/audiodecoder
INSTALLS += target
