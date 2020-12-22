# Generated from qdeclarativemultimediaglobal.pro.

#####################################################################
## tst_qdeclarativemultimediaglobal Test:
#####################################################################

qt_add_test(tst_qdeclarativemultimediaglobal
    GUI
    QMLTEST
    SOURCES
        ../qmultimedia_common/mockcameracapturebuffercontrol.h
        ../qmultimedia_common/mockcameracontrol.h
        ../qmultimedia_common/mockcameraexposurecontrol.h
        ../qmultimedia_common/mockcamerafocuscontrol.h
        ../qmultimedia_common/mockcameraimagecapturecontrol.h
        ../qmultimedia_common/mockcameraimageprocessingcontrol.h
        ../qmultimedia_common/mockcameraservice.h
        ../qmultimedia_common/mockcameraviewfindersettingscontrol.h
        ../qmultimedia_common/mockimageencodercontrol.h
        ../qmultimedia_common/mockmediaobject.h
        ../qmultimedia_common/mockmediaservice.h
        ../qmultimedia_common/mockmediaserviceprovider.h
        ../qmultimedia_common/mockvideodeviceselectorcontrol.h
        ../qmultimedia_common/mockvideorenderercontrol.h
        ../qmultimedia_common/mockvideosurface.h
        ../qmultimedia_common/mockvideowindowcontrol.h
        tst_qdeclarativemultimediaglobal.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/multimedia ../../../../src/multimedia
        ../../../../src/multimedia/camera
        ../../../../src/multimedia/video
        ../../../src/multimedia
        ../../../src/multimedia/video
        ../qmultimedia_common ../qmultimedia_common ../qmultimedia_common
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)

#### Keys ignored in scope 1:.:.:qdeclarativemultimediaglobal.pro:<TRUE>:
# OTHER_FILES = "tst_qdeclarativemultimediaglobal.qml"
