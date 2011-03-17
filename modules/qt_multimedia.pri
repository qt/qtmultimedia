QT_MULTIMEDIA_VERSION = $$QT_VERSION
QT_MULTIMEDIA_MAJOR_VERSION = $$QT_MAJOR_VERSION
QT_MULTIMEDIA_MINOR_VERSION = $$QT_MINOR_VERSION
QT_MULTIMEDIA_PATCH_VERSION = $$QT_PATCH_VERSION

QT.multimedia.name = QtMultimedia
QT.multimedia.bins = $$QT_MODULE_BIN_BASE
QT.multimedia.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtMultimedia
QT.multimedia.private_includes = $$QT_MODULE_INCLUDE_BASE/QtMultimedia/private
QT.multimedia.sources = $$QT_MODULE_BASE/src/multimedia
QT.multimedia.libs = $$QT_MODULE_LIB_BASE
QT.multimedia.imports = $$QT_MODULE_IMPORT_BASE
QT.multimedia.depends = core gui
QT.multimedia.DEFINES = QT_MULTIMEDIA_LIB
