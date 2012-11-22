TEMPLATE = subdirs

SUBDIRS += customvideoitem customvideowidget

# install
sources.files = customvideosurface.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/customvideosurface
INSTALLS += sources
