# Generated from mockserviceplugin5.pro.

#####################################################################
## MockServicePlugin5 Plugin:
#####################################################################

qt_internal_add_plugin(MockServicePlugin5
    OUTPUT_NAME mockserviceplugin5
    TYPE mediaservice
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../$${PLUGIN_TYPE}"
    SOURCES
        ../mockservice.h
        mockserviceplugin5.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
)

#### Keys ignored in scope 1:.:.:mockserviceplugin5.pro:<TRUE>:
# OTHER_FILES = "mockserviceplugin5.json"
# target.path = "$$[QT_INSTALL_TESTS]/tst_qmediaserviceprovider/$${PLUGIN_TYPE}"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:mockserviceplugin5.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug/$${PLUGIN_TYPE}"

#### Keys ignored in scope 4:.:.:mockserviceplugin5.pro:else:
# DESTDIR = "../release/$${PLUGIN_TYPE}"
