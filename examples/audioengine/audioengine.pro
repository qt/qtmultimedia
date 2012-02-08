TEMPLATE = subdirs

# These examples contain no C++ and can simply be copied
SUBDIRS =
sources.files = qml/*
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/declarative/audioengine
INSTALLS += sources

OTHER_FILES += qml/*.qml qml/*.qmlproject qml/content/*

