
TEMPLATE=app

QT += declarative qtquick1 network multimediakit

contains(QT_CONFIG, opengl) {
    QT += opengl
}

SOURCES += $$PWD/qmlcamera.cpp
!mac:TARGET = qml_camera
else:TARGET = QmlCamera

RESOURCES += declarative-camera.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/qml_camera
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/qml_camera

INSTALLS += target sources

