TEMPLATE = app
TARGET += audiooutput

QT += multimediakit

HEADERS       = audiooutput.h

SOURCES       = audiooutput.cpp \
                main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/audiooutput
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/audiooutput

INSTALLS += target sources

