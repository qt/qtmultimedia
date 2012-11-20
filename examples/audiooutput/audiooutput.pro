TEMPLATE = app
TARGET = audiooutput

QT += multimedia widgets

HEADERS       = audiooutput.h

SOURCES       = audiooutput.cpp \
                main.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audiooutput
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audiooutput

INSTALLS += target sources
