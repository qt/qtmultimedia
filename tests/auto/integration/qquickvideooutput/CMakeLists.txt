# Generated from qdeclarativevideooutput.pro.

#####################################################################
## tst_qdeclarativevideooutput Test:
#####################################################################

qt_internal_add_test(tst_qquickvideooutput
    SOURCES
        tst_qquickvideooutput.cpp
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

qt_internal_add_resource(tst_qquickvideooutput "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

