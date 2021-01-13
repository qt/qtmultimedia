# Generated from mockserviceplugin4.pro.

#####################################################################
## MockServicePlugin4 Plugin:
#####################################################################

qt_internal_add_plugin(MockServicePlugin4
    OUTPUT_NAME mockserviceplugin4
    TYPE mediaservice
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../$${PLUGIN_TYPE}"
    SOURCES
        ../mockservice.h
        mockserviceplugin4.cpp
    DEFINES
        QT_DISABLE_DEPRECATED_BEFORE=0
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
)

#### Keys ignored in scope 1:.:.:mockserviceplugin4.pro:<TRUE>:
# OTHER_FILES = "mockserviceplugin4.json"
# target.path = "$$[QT_INSTALL_TESTS]/tst_qmediaserviceprovider/$${PLUGIN_TYPE}"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:mockserviceplugin4.pro:(CMAKE_BUILD_TYPE STREQUAL Debug):
# DESTDIR = "../debug/$${PLUGIN_TYPE}"

#### Keys ignored in scope 4:.:.:mockserviceplugin4.pro:else:
# DESTDIR = "../release/$${PLUGIN_TYPE}"
