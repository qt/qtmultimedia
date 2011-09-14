TEMPLATE = app
TARGET = audiorecorder

QT += multimediakit

HEADERS = \
    audiorecorder.h
  
SOURCES = \
    main.cpp \
    audiorecorder.cpp

maemo*: {
    FORMS += audiorecorder_small.ui
}else {
    FORMS += audiorecorder.ui
}

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/audiorecorder
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/audiorecorder

INSTALLS += target sources

QT+=widgets
