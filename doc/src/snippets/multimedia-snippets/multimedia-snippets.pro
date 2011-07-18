# Doc snippets - compiled for truthiness

TEMPLATE = lib
TARGET = qtmmksnippets

INCLUDEPATH += ../../../../src/global \
               ../../../../src/multimediakit \
               ../../../../src/multimediakit/audio \
               ../../../../src/multimediakit/video \
               ../../../../src/multimediakit/effects

CONFIG += console

qtAddLibrary(QtMultimediaKit)

SOURCES += \
    audio.cpp \
    video.cpp \
    camera.cpp \
    media.cpp
