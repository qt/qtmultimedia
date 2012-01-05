TEMPLATE = subdirs

SUBDIRS += multimedia

# Everything else depends on multimedia
src_qgsttools.subdir = $$IN_PWD/gsttools
src_qgsttools.depends = multimedia

src_qtmmwidgets.subdir = $$IN_PWD/multimediawidgets
src_qtmmwidgets.depends = multimedia

src_plugins.subdir = $$IN_PWD/plugins
src_plugins.depends = multimedia

src_imports.subdir = $$IN_PWD/imports
src_imports.depends = multimedia

SUBDIRS += src_imports

# Optional bits
contains(config_test_gstreamer, yes):SUBDIRS += src_qgsttools
contains(config_test_widgets, yes) {
    SUBDIRS += src_qtmmwidgets

    # If widgets is around, plugins depends on widgets too (imports does not)
    src_plugins.depends += src_qtmmwidgets
}

SUBDIRS += src_plugins

