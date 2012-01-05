TEMPLATE = subdirs

SUBDIRS += multimedia.pro
contains(config_test_widgets, yes): SUBDIRS += multimediawidgets.pro
