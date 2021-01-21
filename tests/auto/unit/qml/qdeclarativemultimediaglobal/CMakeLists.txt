# Generated from qdeclarativemultimediaglobal.pro.

#####################################################################
## tst_qdeclarativemultimediaglobal Test:
#####################################################################

qt_add_test(tst_qdeclarativemultimediaglobal
    GUI
    QMLTEST
    SOURCES
        tst_qdeclarativemultimediaglobal.cpp
    INCLUDE_DIRECTORIES
        ../../mockbackend
    PUBLIC_LIBRARIES
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}
        Qt::Gui
        Qt::MultimediaPrivate
        QtMultimediaMockBackend
)

#### Keys ignored in scope 1:.:.:qdeclarativemultimediaglobal.pro:<TRUE>:
# OTHER_FILES = "tst_qdeclarativemultimediaglobal.qml"
