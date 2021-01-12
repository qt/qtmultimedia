######################################################################
#
# Qt Multimedia
#
######################################################################

TEMPLATE = subdirs
QT_FOR_CONFIG += multimedia-private

qtHaveModule(quick) {
   SUBDIRS += videonode
}

