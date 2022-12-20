TEMPLATE = app
TARGET = screencapture

QT += multimedia multimediawidgets

HEADERS = \
    screencapturepreview.h \
    screenlistmodel.h \
    windowlistmodel.h

SOURCES = \
    main.cpp \
    screencapturepreview.cpp \
    screenlistmodel.cpp \
    windowlistmodel.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/screencapture
INSTALLS += target
