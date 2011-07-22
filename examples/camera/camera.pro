TEMPLATE = app
TARGET = camera

QT += multimediakit multimediakitwidgets

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

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/camera
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/camera

INSTALLS += target sources

