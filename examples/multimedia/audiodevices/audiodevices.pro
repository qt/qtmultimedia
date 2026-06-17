TEMPLATE = app
TARGET = audiodevices

QT += multimedia
CONFIG += add_ios_ffmpeg_libraries

HEADERS       = audiodevices.h

SOURCES       = audiodevices.cpp \
                main.cpp

FORMS        += audiodevicesbase.ui

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/audiodevices
INSTALLS += target

QT+=widgets

macos: QMAKE_INFO_PLIST = Info.qmake.macos.plist
ios: QMAKE_INFO_PLIST = Info.qmake.ios.plist
