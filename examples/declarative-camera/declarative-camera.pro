include (../mobility_examples.pri)

TEMPLATE=app

QT += declarative network

!maemo5 {
    contains(QT_CONFIG, opengl) {
        QT += opengl
    }
}

win32 {
    #required by Qt SDK to resolve Mobility libraries
    CONFIG+=mobility
    MOBILITY+=multimedia
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

