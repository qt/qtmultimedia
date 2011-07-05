TEMPLATE = app
TARGET = camera

QT += multimediakit

HEADERS = \
    camera.h \
    imagesettings.h \
    videosettings.h

SOURCES = \
    main.cpp \
    camera.cpp \
    imagesettings.cpp \
    videosettings.cpp

FORMS += \
    camera.ui \
    videosettings.ui \
    imagesettings.ui

symbian {
    include(camerakeyevent_symbian/camerakeyevent_symbian.pri)
    TARGET.CAPABILITY += UserEnvironment WriteUserData ReadUserData
    TARGET.EPOCHEAPSIZE = 0x20000 0x3000000
    LIBS += -lavkon -leiksrv -lcone -leikcore
}

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/camera
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/camera

INSTALLS += target sources

