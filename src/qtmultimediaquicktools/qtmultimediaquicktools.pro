load(qt_build_config)

TEMPLATE = lib

TARGET = QtMultimediaQuick_p
QT = core quick multimedia-private

!static:DEFINES += QT_MAKEDLL

DEFINES += QT_BUILD_QTMM_QUICK_LIB

# Header files must go inside source directory of a module
# to be installed by syncqt.
INCLUDEPATH += ../multimedia/qtmultimediaquicktools_headers/
DEPENDPATH += ../multimedia/qtmultimediaquicktools_headers/

PRIVATE_HEADERS += \
    qsgvideonode_p.h \
    qtmultimediaquickdefs_p.h

SOURCES += \
    qsgvideonode_p.cpp

HEADERS += $$PRIVATE_HEADERS

DESTDIR = $$QT.multimedia.libs
target.path = $$[QT_INSTALL_LIBS]

INSTALLS += target
