TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += mockbackend multimedia
qtHaveModule(widgets): SUBDIRS += multimediawidgets
qtHaveModule(qml): SUBDIRS += qml
