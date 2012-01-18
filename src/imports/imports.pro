TEMPLATE = subdirs

SUBDIRS += multimedia
contains(config_test_openal, yes): SUBDIRS += audioengine

