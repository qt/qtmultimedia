TEMPLATE = app
TARGET = audiorecorder

QT += multimedia

win32:INCLUDEPATH += $$PWD

HEADERS = \
    audiorecorder.h \
    audiolevel.h

SOURCES = \
    main.cpp \
    audiorecorder.cpp \
    audiolevel.cpp

FORMS += audiorecorder.ui

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/audiorecorder
INSTALLS += target

QT+=widgets
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
