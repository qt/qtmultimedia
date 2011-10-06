QT.multimedia.VERSION = 5.0.0
QT.multimedia.MAJOR_VERSION = 5
QT.multimedia.MINOR_VERSION = 0
QT.multimedia.PATCH_VERSION = 0

QT.multimedia.name = QtMultimedia
QT.multimedia.bins = $$QT_MODULE_BIN_BASE
QT.multimedia.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtMultimedia
QT.multimedia.private_includes = $$QT_MODULE_INCLUDE_BASE/QtMultimedia/$$QT.multimedia.VERSION
QT.multimedia.sources = $$QT_MODULE_BASE/src/multimedia
QT.multimedia.libs = $$QT_MODULE_LIB_BASE
QT.multimedia.plugins = $$QT_MODULE_PLUGIN_BASE
QT.multimedia.imports = $$QT_MODULE_IMPORT_BASE
QT.multimedia.depends = gui network
QT.multimedia.DEFINES = QT_MULTIMEDIA_LIB

QT_CONFIG += multimedia
