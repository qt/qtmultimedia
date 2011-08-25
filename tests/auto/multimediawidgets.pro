
TEMPLATE = subdirs
SUBDIRS += \
    qcamera \
    qcamerabackend \
    qcameraimagecapture \
    qcameraviewfinder \
    qmediaobject \
    qmediaplayer

# This is a commment for the mock backend directory so that maketestselftest
# doesn't believe it's an untested directory
# qmultimedia_common

# Tests depending on private interfaces should only be built if
# these interfaces are exported.
contains (QT_CONFIG, private_tests) {
  SUBDIRS += \
    qgraphicsvideoitem \
    qmediaimageviewer \
    qpaintervideosurface \
    qvideowidget \

    contains (QT_CONFIG, declarative) {
        disabled:SUBDIRS += qdeclarativevideo
    }
}

