TEMPLATE = app
TARGET = videowidget

QT += multimedia multimediawidgets

HEADERS = \
    videoplayer.h

SOURCES = \
    main.cpp \
    videoplayer.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/videowidget
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/videowidget

INSTALLS += target sources

QT+=widgets
