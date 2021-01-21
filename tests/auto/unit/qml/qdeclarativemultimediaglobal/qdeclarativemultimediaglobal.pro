TARGET = tst_qdeclarativemultimediaglobal
CONFIG += warn_on qmltestcase

QT += multimedia-private

include (../../mockbackend/mockbackend.pri)

SOURCES += \
        tst_qdeclarativemultimediaglobal.cpp


OTHER_FILES += \
    tst_qdeclarativemultimediaglobal.qml
