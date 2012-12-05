TEMPLATE = app
TARGET = audiorecorder

QT += multimedia

win32:INCLUDEPATH += $$PWD

HEADERS = \
    audiorecorder.h \
    qaudiolevel.h
  
SOURCES = \
    main.cpp \
    audiorecorder.cpp \
    qaudiolevel.cpp

maemo*: {
    FORMS += audiorecorder_small.ui
}else {
    FORMS += audiorecorder.ui
}

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/audiorecorder
INSTALLS += target

QT+=widgets
