# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
    qmockaudiodecoder.cpp qmockaudiodecoder.h
    qmockaudiooutput.h
    qmockcamera.cpp qmockcamera.h
    qmockimagecapture.cpp qmockimagecapture.h
    qmockmediaplayer.h
    qmockmediaencoder.h
    qmockmediacapturesession.h
    qmockvideosink.h
    qmockmediadevices.cpp qmockmediadevices.h
    qmockintegration.cpp qmockintegration.h
)
# special case end

#### Keys ignored in scope 1:.:.:mockbackend.pro:<TRUE>:
# TEMPLATE = "lib"
