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
