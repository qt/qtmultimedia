TARGET = tst_qdeclarativecamera
CONFIG += warn_on qmltestcase

QT += multimedia-private

include (../../mockbackend/mockbackend.pri)

SOURCES += \
        tst_qdeclarativecamera.cpp

OTHER_FILES += \
    tst_qdeclarativecamera.qml
