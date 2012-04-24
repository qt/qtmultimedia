
TEMPLATE=app

QT += qtquick1 network multimedia

contains(QT_CONFIG, opengl) {
    QT += opengl
}

SOURCES += $$PWD/qmlcamera.cpp
!mac:TARGET = qml_camera
else:TARGET = QmlCamera

RESOURCES += declarative-camera.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/qml_camera
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/qml_camera

INSTALLS += target sources

QT+=widgets
