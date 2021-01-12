TEMPLATE = subdirs

SUBDIRS += multimedia

include($$OUT_PWD/multimedia/qtmultimedia-config.pri)
QT_FOR_CONFIG += multimedia-private

# Everything else depends on multimedia
src_qtmmwidgets.subdir = multimediawidgets
src_qtmmwidgets.depends = multimedia

src_plugins.subdir = plugins
src_plugins.depends = multimedia

android:SUBDIRS += android

qtHaveModule(quick) {
    src_qtmultimediaquicktools.subdir = qtmultimediaquicktools
    src_qtmultimediaquicktools.depends = multimedia

    src_imports.subdir = imports
    src_imports.depends = multimedia src_qtmultimediaquicktools

    # For the videonode plugin
    src_plugins.depends += src_qtmultimediaquicktools

    SUBDIRS += \
        src_qtmultimediaquicktools \
        src_imports
}

# Optional bits
qtHaveModule(widgets) {
    SUBDIRS += src_qtmmwidgets

    # If widgets is around, plugins depends on widgets too (imports does not)
    src_plugins.depends += src_qtmmwidgets
}

SUBDIRS += src_plugins

