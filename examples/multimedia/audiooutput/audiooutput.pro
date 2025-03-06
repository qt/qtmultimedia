TEMPLATE = app
TARGET = audiooutput

QT += multimedia widgets
CONFIG += add_ios_ffmpeg_libraries

HEADERS       = audiooutput.h

SOURCES       = audiooutput.cpp \
                main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/audiooutput
INSTALLS += target
