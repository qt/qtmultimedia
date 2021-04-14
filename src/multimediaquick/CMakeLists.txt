# Generated from qtmultimediaquicktools.pro.

#####################################################################
## MultimediaQuick Module:
#####################################################################

qt_add_module(MultimediaQuick
    CONFIG_MODULE_NAME multimediaquick
    INTERNAL_MODULE
    SOURCES
        qdeclarativevideooutput.cpp qdeclarativevideooutput_p.h
        qdeclarativevideooutput_render.cpp qdeclarativevideooutput_render_p.h
        qsgvideonode_p.cpp qsgvideonode_p.h
        qsgvideotexture.cpp qsgvideotexture_p.h
        qtmultimediaquickglobal_p.h
        qtmultimediaquicktypes_p.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::MultimediaPrivate
        Qt::Quick
        Qt::QuickPrivate
)

set_target_properties(MultimediaQuick PROPERTIES
    QT_QML_MODULE_INSTALL_QMLTYPES TRUE
    QT_QML_MODULE_VERSION ${CMAKE_PROJECT_VERSION}
    QT_QML_MODULE_URI QtMultimedia
    QT_QMLTYPES_FILENAME plugins.qmltypes
    QT_QML_MODULE_INSTALL_DIR "${INSTALL_QMLDIR}/QtMultimedia"
)

qt6_qml_type_registration(MultimediaQuick)
