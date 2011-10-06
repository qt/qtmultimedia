TEMPLATE = app
TARGET = slideshow

QT += multimedia multimediawidgets

HEADERS = \
    slideshow.h

SOURCES = \
    main.cpp \
    slideshow.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/slideshow
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/slideshow

INSTALLS += target sources

QT+=widgets
