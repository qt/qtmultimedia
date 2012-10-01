TEMPLATE = app
TARGET = customvideoitem

QT += multimedia multimediawidgets

contains(QT_CONFIG, opengl): QT += opengl

HEADERS   += videoplayer.h \
             videoitem.h

SOURCES   += main.cpp \
             videoplayer.cpp \
             videoitem.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/customvideoitem
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/customvideoitem

INSTALLS += target sources

QT+=widgets
