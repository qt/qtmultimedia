TEMPLATE = app
TARGET = audiosource

QT += multimedia widgets
CONFIG += add_ios_ffmpeg_libraries

HEADERS       = audiosource.h

SOURCES       = audiosource.cpp \
                main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/audiosource
INSTALLS += target

macos: QMAKE_INFO_PLIST = Info.qmake.macos.plist
ios: QMAKE_INFO_PLIST = Info.qmake.ios.plist
