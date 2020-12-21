# Generated from qmediaplaylist.pro.

#####################################################################
## tst_qmediaplaylist Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND test_data ${test_data_glob})

qt_add_test(tst_qmediaplaylist
    SOURCES
        tst_qmediaplaylist.cpp
    INCLUDE_DIRECTORIES
        ../../../../src/plugins/m3u
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
    TESTDATA ${test_data}
)
