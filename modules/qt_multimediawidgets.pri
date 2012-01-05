QT.multimediawidgets.VERSION = 5.0.0
QT.multimediawidgets.MAJOR_VERSION = 5
QT.multimediawidgets.MINOR_VERSION = 0
QT.multimediawidgets.PATCH_VERSION = 0

QT.multimediawidgets.name = QtMultimediaWidgets
QT.multimediawidgets.bins = $$QT_MODULE_BIN_BASE
QT.multimediawidgets.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtMultimediaWidgets
QT.multimediawidgets.private_includes = $$QT_MODULE_INCLUDE_BASE/QtMultimediaWidgets/$$QT.multimediawidgets.VERSION
QT.multimediawidgets.sources = $$QT_MODULE_BASE/src/multimediawidgets
QT.multimediawidgets.libs = $$QT_MODULE_LIB_BASE
QT.multimediawidgets.plugins = $$QT_MODULE_PLUGIN_BASE
QT.multimediawidgets.imports = $$QT_MODULE_IMPORT_BASE
QT.multimediawidgets.depends = gui network widgets
QT.multimediawidgets.DEFINES = QT_MULTIMEDIAWIDGETS_LIB

QT_CONFIG += multimediawidgets
