INCLUDEPATH += playback

PUBLIC_HEADERS += \
    playback/qmediacontent.h \
    playback/qmediaplayer.h \
    playback/qmediaplaylist.h

PRIVATE_HEADERS += \
    playback/qmediaplaylist_p.h \
    playback/qplaylistfileparser_p.h

SOURCES += \
    playback/qmediacontent.cpp \
    playback/qmediaplayer.cpp \
    playback/qmediaplaylist.cpp \
    playback/qplaylistfileparser.cpp
