QT += qml quick multimedia

SOURCES += main.cpp
RESOURCES += declarative-radio.qrc

OTHER_FILES += view.qml

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/declarative-radio
sources.files = $$SOURCES $$HEADERS $$RESOURCES *.pro doc view.qml
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/declarative-radio
INSTALLS += target sources
