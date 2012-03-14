TEMPLATE = subdirs
CONFIG += ORDERED

SUBDIRS += \
    test \
    mockserviceplugin1 \
    mockserviceplugin2 \
    mockserviceplugin3 \
    mockserviceplugin4

# no special install rule for subdir
INSTALLS =

