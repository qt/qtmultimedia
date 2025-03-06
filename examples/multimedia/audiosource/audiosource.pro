TEMPLATE = app
TARGET = audiosource

QT += multimedia widgets
CONFIG += add_ios_ffmpeg_libraries

HEADERS       = audiosource.h

SOURCES       = audiosource.cpp \
                main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/audiosource
INSTALLS += target
include(../shared/shared.pri)
