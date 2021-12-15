TEMPLATE = app
TARGET = camera

QT += multimedia multimediawidgets

HEADERS = \
    camera.h \
    imagesettings.h \
    videosettings.h \
    metadatadialog.h

SOURCES = \
    main.cpp \
    camera.cpp \
    imagesettings.cpp \
    videosettings.cpp \
    metadatadialog.cpp

FORMS += \
    imagesettings.ui

android|ios {
    FORMS += \
        camera_mobile.ui \
        videosettings_mobile.ui
} else {
    FORMS += \
        camera.ui \
        videosettings.ui
}
RESOURCES += camera.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/multimediawidgets/camera
INSTALLS += target

QT += widgets
include(../../multimedia/shared/shared.pri)

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
OTHER_FILES += android/AndroidManifest.xml

macos {
    macx-xcode {
        code_sign_entitlements.name = CODE_SIGN_ENTITLEMENTS
        code_sign_entitlements.value = $$PWD/$${TARGET}.entitlements
        QMAKE_MAC_XCODE_SETTINGS += code_sign_entitlements
    } else {
        QMAKE_POST_LINK += "codesign --sign - --entitlements $$PWD/$${TARGET}.entitlements $${OUT_PWD}/$${TARGET}.app"
    }
}
