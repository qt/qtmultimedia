# Generated from qwavedecoder.pro.

#####################################################################
## tst_qwavedecoder Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qwavedecoder
    SOURCES
        ../../../../src/multimedia/audio/qwavedecoder_p.cpp ../../../../src/multimedia/audio/qwavedecoder_p.h
        tst_qwavedecoder.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
    TESTDATA ${test_data}
)
