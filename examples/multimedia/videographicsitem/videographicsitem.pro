TEMPLATE = app
TARGET = videographicsitem

QT += multimedia multimediawidgets

HEADERS   += videoplayer.h

SOURCES   += main.cpp \
             videoplayer.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/videographicsitem
INSTALLS += target

QT+=widgets
