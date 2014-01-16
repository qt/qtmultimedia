
TEMPLATE = subdirs
SUBDIRS += \
    qdeclarativeaudio \

disabled {
    SUBDIRS += \
        qdeclarativevideo
}

