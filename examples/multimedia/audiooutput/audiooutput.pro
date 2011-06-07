HEADERS       = audiooutput.h
SOURCES       = audiooutput.cpp \
                main.cpp

QT           += multimedia

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/multimedia/audiooutput
sources.files = $$SOURCES *.h $$RESOURCES $$FORMS audiooutput.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/multimedia/audiooutput
INSTALLS += target sources

symbian {
    TARGET.UID3 = 0xA000D7C0
    CONFIG += qt_example
}
maemo5: CONFIG += qt_example
maemo5: warning(This example might not fully work on Maemo platform)
