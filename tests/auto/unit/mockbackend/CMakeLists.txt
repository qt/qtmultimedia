# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## MockMultimediaPlugin Generic Library:
#####################################################################

qt_internal_add_plugin(MockMultimediaPlugin
    STATIC
    OUTPUT_NAME mockmultimediaplugin
    PLUGIN_TYPE multimedia
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../plugins"
    DEFAULT_IF FALSE
    TEST_PLUGIN
    SOURCES
        qmockaudiodecoder.cpp qmockaudiodecoder.h
        qmockaudiooutput.h
        qmockcamera.cpp qmockcamera.h
        qmockimagecapture.cpp qmockimagecapture.h
        qmockmediaplayer.h
        qmockmediaencoder.h
        qmockmediacapturesession.h
        qmockvideosink.h
        qmockaudiodevices.cpp qmockaudiodevices.h
        qmockvideodevices.cpp qmockvideodevices.h
        qmockintegration.cpp qmockintegration.h
    LIBRARIES
        Qt::MultimediaPrivate
        Qt::CorePrivate
)
