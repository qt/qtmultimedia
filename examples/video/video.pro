TEMPLATE = subdirs

SUBDIRS += qmlvideo qmlvideofx

# install
sources.files = video.pro doc snippets
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/video
INSTALLS += sources
