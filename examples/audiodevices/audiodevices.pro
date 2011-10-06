TEMPLATE = app
TARGET = audiodevices

QT += multimedia

HEADERS       = audiodevices.h

SOURCES       = audiodevices.cpp \
                main.cpp

FORMS        += audiodevicesbase.ui

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audiodevices
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audiodevices

INSTALLS += target sources

QT+=widgets
