TEMPLATE = app
TARGET = audiodevices

QT += multimediakit

HEADERS       = audiodevices.h

SOURCES       = audiodevices.cpp \
                main.cpp

FORMS        += audiodevicesbase.ui

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/audiodevices
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/audiodevices

INSTALLS += target sources

