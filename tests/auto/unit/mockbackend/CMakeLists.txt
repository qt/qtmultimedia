# Generated from mockbackend.pro.

#####################################################################
## QtMultimediaMockBackend Generic Library:
#####################################################################

# special case begin
add_library(QtMultimediaMockBackend INTERFACE)
target_include_directories(QtMultimediaMockBackend INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(QtMultimediaMockBackend INTERFACE
    Qt::Core
    Qt::Gui
    Qt::MultimediaPrivate
)
target_sources(QtMultimediaMockBackend INTERFACE
    mockaudiodecodercontrol.h
    mockaudiodecoderservice.h
    mockcameracontrol.h
    mockcameraexposurecontrol.h
    mockcamerafocuscontrol.h
    mockcameraimagecapturecontrol.h
    mockcameraimageprocessingcontrol.h
    mockmediaplayercontrol.h
    mockmediaplayerservice.h
    mockmediarecordercontrol.h
    mockmediarecorderservice.h
    mockmediaservice.h
    mockmediasource.h
    mockmediastreamscontrol.h
    mockmetadatareadercontrol.h
    mockmetadatawritercontrol.h
    mockvideorenderercontrol.h
    mockvideosurface.h
    mockvideowindowcontrol.h
    qmockdevicemanager.cpp
    qmockdevicemanager_p.h
    qmockintegration.cpp
    qmockintegration_p.h
)
# special case end

#### Keys ignored in scope 1:.:.:mockbackend.pro:<TRUE>:
# TEMPLATE = "lib"
