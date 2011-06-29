
TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += multimediakit
SUBDIRS += imports
SUBDIRS += plugins

symbian {
    SUBDIRS += s60installs
}

