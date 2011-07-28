QT.multimediakit.VERSION = 5.0.0
QT.multimediakit.MAJOR_VERSION = 5
QT.multimediakit.MINOR_VERSION = 0
QT.multimediakit.PATCH_VERSION = 0

QT.multimediakit.name = QtMultimediaKit
QT.multimediakit.bins = $$QT_MODULE_BIN_BASE
QT.multimediakit.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtMultimediaKit
QT.multimediakit.private_includes = $$QT_MODULE_INCLUDE_BASE/QtMultimediaKit/$$QT.multimediakit.VERSION
QT.multimediakit.sources = $$QT_MODULE_BASE/src/multimediakit
QT.multimediakit.libs = $$QT_MODULE_LIB_BASE
QT.multimediakit.plugins = $$QT_MODULE_PLUGIN_BASE
QT.multimediakit.imports = $$QT_MODULE_IMPORT_BASE
QT.multimediakit.depends = gui network
QT.multimediakit.DEFINES = QT_MULTIMEDIAKIT_LIB

QT_CONFIG += multimediakit
