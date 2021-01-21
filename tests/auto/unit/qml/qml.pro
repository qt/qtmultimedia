
TEMPLATE = subdirs
SUBDIRS += \
    qdeclarativemultimediaglobal \
    qdeclarativeaudio \

disabled {
    SUBDIRS += \
        qdeclarativevideo
}

