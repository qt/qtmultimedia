TEMPLATE = app
TARGET = videowidget

QT += multimediakit multimediakitwidgets

HEADERS = \
    videoplayer.h \
    videowidget.h \
    videowidgetsurface.h

SOURCES = \
    main.cpp \
    videoplayer.cpp \
    videowidget.cpp \
    videowidgetsurface.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/videowidget
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/videowidget

INSTALLS += target sources

QT+=widgets
