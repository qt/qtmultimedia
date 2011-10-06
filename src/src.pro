
TEMPLATE = subdirs
CONFIG += ordered

library_qtmmwidgets.subdir = $$IN_PWD/multimediawidgets
library_qtmmwidgets.depends = multimedia

SUBDIRS += multimedia
SUBDIRS += library_qtmmwidgets
SUBDIRS += imports
SUBDIRS += plugins

