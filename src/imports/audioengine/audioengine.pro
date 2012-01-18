TARGET  = declarative_audioengine
TARGETPATH = QtAudioEngine

include(../qimportbase.pri)
QT += declarative quick multimedia-private

win32 {
    LIBS += -lOpenAL32
}else {
    LIBS += -lopenal
}

DESTDIR = $$QT.multimedia.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INCLUDEPATH += ../../multimedia/audio

HEADERS += \
        qdeclarative_attenuationmodel_p.h \
        qdeclarative_audioengine_p.h \
        qdeclarative_soundinstance_p.h \
        qdeclarative_audiocategory_p.h \
        qdeclarative_audiolistener_p.h \
        qdeclarative_playvariation_p.h \
        qdeclarative_audiosample_p.h \
        qdeclarative_sound_p.h \
        qsoundinstance_p.h \
        qaudioengine_p.h \
        qsoundsource_p.h \
        qsoundbuffer_p.h \
        qaudioengine_openal_p.h

SOURCES += \
        audioengine.cpp \
        qdeclarative_attenuationmodel_p.cpp \
        qdeclarative_audioengine_p.cpp \
        qdeclarative_soundinstance_p.cpp \
        qdeclarative_audiocategory_p.cpp \
        qdeclarative_audiolistener_p.cpp \
        qdeclarative_playvariation_p.cpp \
        qdeclarative_audiosample_p.cpp \
        qdeclarative_sound_p.cpp \
        qsoundinstance_p.cpp \
        qaudioengine_p.cpp \
        qsoundsource_openal_p.cpp  \
        qaudioengine_openal_p.cpp

qmldir.files += $$PWD/qmldir
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
