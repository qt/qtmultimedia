TEMPLATE = app
TARGET = audiodecoder

CONFIG += qt warn_on

HEADERS = \
    audiodecoder.h \
    wavefilewriter.h
SOURCES = main.cpp \
    audiodecoder.cpp \
    wavefilewriter.cpp

QT += multimedia
CONFIG += console

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audiodecoder
sources.files = $$SOURCES $$HEADERS audiodecoder.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audiodecoder
INSTALLS += target sources
