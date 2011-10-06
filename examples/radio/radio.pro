TEMPLATE = app
TARGET = radio

QT += multimedia

HEADERS = \
    radio.h
  
SOURCES = \
    main.cpp \
    radio.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/radio
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/radio

INSTALLS += target sources

QT+=widgets
