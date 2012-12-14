TARGET = QtMultimediaQuick_p
QT = core quick multimedia-private
CONFIG += internal_module

load(qt_module)

DEFINES += QT_BUILD_QTMM_QUICK_LIB

# Header files must go inside source directory of a module
# to be installed by syncqt.
INCLUDEPATH += ../multimedia/qtmultimediaquicktools_headers/

PRIVATE_HEADERS += \
    ../multimedia/qtmultimediaquicktools_headers/qsgvideonode_p.h \
    ../multimedia/qtmultimediaquicktools_headers/qtmultimediaquickdefs_p.h

SOURCES += \
    qsgvideonode_p.cpp

HEADERS += $$PRIVATE_HEADERS
