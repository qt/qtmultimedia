TEMPLATE = subdirs

SUBDIRS += multimedia.pro
contains(QT_CONFIG,multimediawidgets): SUBDIRS += multimediawidgets.pro
