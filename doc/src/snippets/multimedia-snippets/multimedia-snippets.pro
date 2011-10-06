# Doc snippets - compiled for truthiness

TEMPLATE = lib
TARGET = qtmmksnippets

INCLUDEPATH += ../../../../src/global \
               ../../../../src/multimedia \
               ../../../../src/multimedia/audio \
               ../../../../src/multimedia/video \
               ../../../../src/multimedia/effects

CONFIG += console

QT += multimedia

SOURCES += \
    audio.cpp \
    video.cpp \
    camera.cpp \
    media.cpp
