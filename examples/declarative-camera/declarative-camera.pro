TEMPLATE=app
TARGET=declarative-camera

QT += quick qml multimedia

SOURCES += qmlcamera.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/declarative-camera

qml.files = declarative-camera.qml \
            CameraButton.qml \
            CameraPropertyButton.qml \
            CameraPropertyPopup.qml \
            FocusButton.qml \
            PhotoCaptureControls.qml \
            PhotoPreview.qml \
            VideoCaptureControls.qml \
            VideoPreview.qml \
            ZoomControl.qml \
            images

qml.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/declarative-camera

INSTALLS += target qml
