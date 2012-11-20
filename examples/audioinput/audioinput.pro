TEMPLATE = app
TARGET = audioinput

QT += multimedia widgets

HEADERS       = audioinput.h

SOURCES       = audioinput.cpp \
                main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audioinput
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audioinput

INSTALLS += target sources
