TEMPLATE = subdirs

!static {
    SUBDIRS += \
        audiodevices \
        audioinput \
        audiooutput
}

SUBDIRS += \
    videographicsitem \
    videowidget

# install
target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/multimedia
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS multimedia.pro README
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/multimedia
INSTALLS += target sources
