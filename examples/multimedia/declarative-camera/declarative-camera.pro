TEMPLATE=app
TARGET=declarative-camera

QT += quick qml multimedia

SOURCES += qmlcamera.cpp
RESOURCES += declarative-camera.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/declarative-camera
INSTALLS += target
include(../shared/shared.pri)

macos {
    macx-xcode {
        code_sign_entitlements.name = CODE_SIGN_ENTITLEMENTS
        code_sign_entitlements.value = $$PWD/$${TARGET}.entitlements
        QMAKE_MAC_XCODE_SETTINGS += code_sign_entitlements
    } else {
        QMAKE_POST_LINK += "codesign --sign - --entitlements $$PWD/$${TARGET}.entitlements $${OUT_PWD}/$${TARGET}.app"
    }
}
