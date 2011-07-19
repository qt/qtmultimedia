TEMPLATE = app
TARGET = slideshow

QT += multimediakit

HEADERS = \
    slideshow.h

SOURCES = \
    main.cpp \
    slideshow.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/slideshow
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimediakit/slideshow

INSTALLS += target sources

