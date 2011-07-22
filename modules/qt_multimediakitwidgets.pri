QT.multimediakitwidgets.VERSION = 5.0.0
QT.multimediakitwidgets.MAJOR_VERSION = 5
QT.multimediakitwidgets.MINOR_VERSION = 0
QT.multimediakitwidgets.PATCH_VERSION = 0

QT.multimediakitwidgets.name = QtMultimediaKitWidgets
QT.multimediakitwidgets.bins = $$QT_MODULE_BIN_BASE
QT.multimediakitwidgets.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtMultimediaKitWidgets
QT.multimediakitwidgets.private_includes = $$QT_MODULE_INCLUDE_BASE/QtMultimediaKitWidgets/$$QT.multimediakitwidgets.VERSION
QT.multimediakitwidgets.sources = $$QT_MODULE_BASE/src/multimediakitwidgets
QT.multimediakitwidgets.libs = $$QT_MODULE_LIB_BASE
QT.multimediakitwidgets.plugins = $$QT_MODULE_PLUGIN_BASE
QT.multimediakitwidgets.imports = $$QT_MODULE_IMPORT_BASE
QT.multimediakitwidgets.depends = gui network
QT.multimediakitwidgets.DEFINES = QT_MULTIMEDIAKITWIDGETS_LIB

QT_CONFIG += multimediakitwidgets
