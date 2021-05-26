# Generated from qmediaplayerbackend.pro.

#####################################################################
## tst_qmediaplayerbackend Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    testdata/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmediaplayerbackend
    SOURCES
        ../shared/mediafileselector.h
        tst_qmediaplayerbackend.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmediaplayerbackend CONDITION boot2qt
    DEFINES
        SKIP_OGV_TEST
)
