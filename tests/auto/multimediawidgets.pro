
TEMPLATE = subdirs
SUBDIRS += \
    qcameraviewfinder \
    qcamerawidgets \
    qmediaplayerwidgets \

# This is a commment for the mock backend directory so that maketestselftest
# doesn't believe it's an untested directory
# qmultimedia_common

# Tests depending on private interfaces should only be built if
# these interfaces are exported.
contains (QT_CONFIG, private_tests) {
  SUBDIRS += \
    qgraphicsvideoitem \
    qpaintervideosurface \
    qmediaimageviewerwidgets \
    qvideowidget \

    contains (QT_CONFIG, declarative) {
        disabled:SUBDIRS += qdeclarativevideo
    }
}

