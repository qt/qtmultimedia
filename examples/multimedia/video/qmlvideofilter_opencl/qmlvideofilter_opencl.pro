TEMPLATE = app
TARGET = qmlvideofilter_opencl

QT += quick multimedia

SOURCES = main.cpp
HEADERS = rgbframehelper.h

RESOURCES = qmlvideofilter_opencl.qrc
OTHER_FILES = main.qml

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/video/qmlvideofilter_opencl
INSTALLS += target

# Edit these as necessary
osx {
    LIBS += -framework OpenCL
} else {
    INCLUDEPATH += c:/cuda/include
    LIBPATH += c:/cuda/lib/x64
    LIBS += -lopengl32 -lOpenCL
}
