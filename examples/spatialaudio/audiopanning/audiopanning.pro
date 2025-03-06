TEMPLATE = app
TARGET = audiopanning

QT += multimedia widgets spatialaudio
CONFIG += add_ios_ffmpeg_libraries

SOURCES       = main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/spatialaudio/audiopanning
INSTALLS += target
