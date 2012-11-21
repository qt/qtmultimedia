TEMPLATE = app
TARGET = player

QT += network \
      xml \
      multimedia \
      multimediawidgets \
      widgets

HEADERS = \
    player.h \
    playercontrols.h \
    playlistmodel.h \
    videowidget.h \
    histogramwidget.h
SOURCES = main.cpp \
    player.cpp \
    playercontrols.cpp \
    playlistmodel.cpp \
    videowidget.cpp \
    histogramwidget.cpp

maemo* {
    DEFINES += PLAYER_NO_COLOROPTIONS
}

target.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/player
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/qtmultimedia/player
INSTALLS += target sources
