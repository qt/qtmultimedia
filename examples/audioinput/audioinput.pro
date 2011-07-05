TEMPLATE = app
TARGET = audioinput

QT += multimediakit

HEADERS       = audioinput.h

SOURCES       = audioinput.cpp \
                main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/audioinput
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/audioinput

INSTALLS += target sources

