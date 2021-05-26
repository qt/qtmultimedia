# Generated from qsamplecache.pro.

#####################################################################
## tst_qsamplecache Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qsamplecache
    SOURCES
        tst_qsamplecache.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
    TESTDATA ${test_data}
)
