TEMPLATE = app
TARGET = player

QT += network \
      multimedia \
      multimediawidgets \
      widgets

HEADERS = \
    player.h \
    playercontrols.h \
    playlistmodel.h \
    videowidget.h \
    qmediaplaylist.h \
    qmediaplaylist_p.h \
    qplaylistfileparser.h

SOURCES = main.cpp \
    player.cpp \
    playercontrols.cpp \
    playlistmodel.cpp \
    videowidget.cpp \
    qmediaplaylist.cpp \
    qmediaplaylist_p.cpp \
    qplaylistfileparser.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/player
INSTALLS += target
