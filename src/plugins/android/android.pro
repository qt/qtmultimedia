TEMPLATE = subdirs

SUBDIRS += src \
           jar

qtHaveModule(quick) {
    SUBDIRS += videonode
}
