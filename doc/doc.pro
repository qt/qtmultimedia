######################################################################
#
# Mobility API project
#
######################################################################

TEMPLATE = subdirs

# Doc snippets use widgets
!isEmpty(QT.widgets.name): SUBDIRS += src/snippets
