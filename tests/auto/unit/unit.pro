TEMPLATE = subdirs

SUBDIRS += multimedia.pro
!isEmpty(QT.widgets.name): SUBDIRS += multimediawidgets.pro
