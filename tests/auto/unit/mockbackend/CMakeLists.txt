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
    qmockaudiodecoder.h
    qmockcamera.h
    qmockcameraexposure.h
    qmockcamerafocus.h
    qmockcameraimageprocessing.h
    qmockimagecapture.h qmockimagecapture.cpp
    qmockmediaplayer.h
    qmockmediaencoder.h
    qmockmediacapturesession.h
    qmockvideosink.h
    qmockdevicemanager.cpp
    qmockdevicemanager_p.h
    qmockintegration.cpp
    qmockintegration_p.h
)
# special case end

#### Keys ignored in scope 1:.:.:mockbackend.pro:<TRUE>:
# TEMPLATE = "lib"
