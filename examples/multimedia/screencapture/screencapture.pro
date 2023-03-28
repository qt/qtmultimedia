TEMPLATE = app
TARGET = screencapture

QT += multimedia multimediawidgets

HEADERS = \
    screencapturepreview.h \
    screenlistmodel.h

SOURCES = \
    main.cpp \
    screencapturepreview.cpp \
    screenlistmodel.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/screencapture
INSTALLS += target
