# Generated from qdeclarativevideooutput.pro.

#####################################################################
## tst_qdeclarativevideooutput Test:
#####################################################################

qt_add_test(tst_qdeclarativevideooutput
    SOURCES
        tst_qdeclarativevideooutput.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/imports/multimedia
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::MultimediaQuickPrivate
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qml_resource_files
    "main.qml"
)

qt_add_resource(tst_qdeclarativevideooutput "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

