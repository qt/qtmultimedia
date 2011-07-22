
TEMPLATE = subdirs
CONFIG += ordered

library_qtmmkwidgets.subdir = $$IN_PWD/multimediakitwidgets
library_qtmmkwidgets.depends = multimediakit

SUBDIRS += multimediakit
SUBDIRS += library_qtmmkwidgets
SUBDIRS += imports
SUBDIRS += plugins

