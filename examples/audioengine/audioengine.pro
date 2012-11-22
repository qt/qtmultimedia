TEMPLATE = subdirs

# These examples contain no C++ and can simply be copied
SUBDIRS =
sources.files = doc qml
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/audioengine
INSTALLS += sources

OTHER_FILES += qml/*.qml qml/*.qmlproject qml/content/*

