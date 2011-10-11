TEMPLATE = subdirs
CONFIG += ordered

library_qgsttools.subdir = $$IN_PWD/gsttools
library_qgsttools.depends = multimedia

library_qtmmwidgets.subdir = $$IN_PWD/multimediawidgets
library_qtmmwidgets.depends = multimedia

SUBDIRS += multimedia

contains(config_test_gstreamer, yes) {
    SUBDIRS += library_qgsttools
}

SUBDIRS += library_qtmmwidgets
SUBDIRS += imports
SUBDIRS += plugins

