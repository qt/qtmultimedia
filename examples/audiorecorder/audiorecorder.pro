TEMPLATE = app
TARGET = audiorecorder

QT += multimedia

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

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audiorecorder
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audiorecorder

INSTALLS += target sources

QT+=widgets
