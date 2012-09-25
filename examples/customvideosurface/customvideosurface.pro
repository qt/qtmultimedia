TEMPLATE = app
TARGET = customvideosurface

QT += multimedia multimediawidgets

HEADERS = \
    videoplayer.h \
    videowidget.h \
    videowidgetsurface.h

SOURCES = \
    main.cpp \
    videoplayer.cpp \
    videowidget.cpp \
    videowidgetsurface.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/customvideosurface
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/customvideosurface

INSTALLS += target sources

QT+=widgets
