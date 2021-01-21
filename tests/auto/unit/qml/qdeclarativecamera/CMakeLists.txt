# Generated from qdeclarativecamera.pro.

#####################################################################
## tst_qdeclarativecamera Test:
#####################################################################

qt_add_test(tst_qdeclarativecamera
    GUI
    QMLTEST
    SOURCES
        tst_qdeclarativecamera.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)

#### Keys ignored in scope 1:.:.:qdeclarativecamera.pro:<TRUE>:
# OTHER_FILES = "tst_qdeclarativecamera.qml"
