TEMPLATE = app
TARGET = videowidget

QT += multimedia multimediawidgets
CONFIG += add_ios_ffmpeg_libraries

HEADERS = \
    videoplayer.h

SOURCES = \
    main.cpp \
    videoplayer.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/videowidget
INSTALLS += target

QT+=widgets
