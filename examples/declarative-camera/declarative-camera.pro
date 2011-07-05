
TEMPLATE=app

QT += declarative network multimediakit

!maemo5 {
    contains(QT_CONFIG, opengl) {
        QT += opengl
    }
}

SOURCES += $$PWD/qmlcamera.cpp
!mac:TARGET = qml_camera
else:TARGET = QmlCamera

RESOURCES += declarative-camera.qrc

symbian {
    include(camerakeyevent_symbian/camerakeyevent_symbian.pri)
    load(data_caging_paths)
    TARGET.CAPABILITY += UserEnvironment NetworkServices Location ReadUserData WriteUserData
    TARGET.EPOCHEAPSIZE = 0x20000 0x3000000
}

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/qml_camera
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/qml_camera

INSTALLS += target sources

