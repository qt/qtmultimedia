HEADERS       = audioinput.h
SOURCES       = audioinput.cpp \
                main.cpp

QT           += multimedia

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/multimedia/audioinput
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS audioinput.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/multimedia/audioinput
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000D7BF
    TARGET.CAPABILITY += UserEnvironment
    CONFIG += qt_example
}
