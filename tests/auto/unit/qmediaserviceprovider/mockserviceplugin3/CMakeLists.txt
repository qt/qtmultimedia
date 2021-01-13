# Generated from mockserviceplugin3.pro.

#####################################################################
## MockServicePlugin3 Plugin:
#####################################################################

qt_internal_add_plugin(MockServicePlugin3
    OUTPUT_NAME mockserviceplugin3
    TYPE mediaservice
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../$${PLUGIN_TYPE}"
    SOURCES
        ../mockservice.h
        mockserviceplugin3.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
)

#### Keys ignored in scope 1:.:.:mockserviceplugin3.pro:<TRUE>:
# OTHER_FILES = "mockserviceplugin3.json"
# target.path = "$$[QT_INSTALL_TESTS]/tst_qmediaserviceprovider/$${PLUGIN_TYPE}"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:mockserviceplugin3.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug/$${PLUGIN_TYPE}"

#### Keys ignored in scope 4:.:.:mockserviceplugin3.pro:else:
# DESTDIR = "../release/$${PLUGIN_TYPE}"
