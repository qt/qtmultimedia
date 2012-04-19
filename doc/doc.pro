######################################################################
#
# Mobility API project
#
######################################################################

TEMPLATE = subdirs

# Doc snippets use widgets
!isEmpty(QT.widgets.name): SUBDIRS += src/snippets

OTHER_FILES += \
    src/audioengineoverview.qdoc \
    src/audiooverview.qdoc \
    src/cameraoverview.qdoc \
    src/changes.qdoc \
    src/multimediabackend.qdoc \
    src/multimedia.qdoc \
    src/qtmultimedia5.qdoc \
    src/radiooverview.qdoc \
    src/videooverview.qdoc \
    src/examples/audiodevices.qdoc \
    src/examples/audioengine.qdoc \
    src/examples/audioinput.qdoc \
    src/examples/audiooutput.qdoc \
    src/examples/audiorecorder.qdoc \
    src/examples/camera.qdoc \
    src/examples/declarative-camera.qdoc \
    src/examples/declarative-radio.qdoc \
    src/examples/player.qdoc \
    src/examples/qmlvideofx.qdoc \
    src/examples/qmlvideo.qdoc \
    src/examples/spectrum.qdoc \
    src/examples/videographicsitem.qdoc \
    src/examples/videowidget.qdoc
