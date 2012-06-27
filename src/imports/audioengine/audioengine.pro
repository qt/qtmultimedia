TARGET  = declarative_audioengine
TARGETPATH = QtAudioEngine

include(../qimportbase.pri)
QT += quick qml multimedia-private

win32: LIBS += -lOpenAL32
unix:!mac: LIBS += -lopenal
mac: LIBS += -framework OpenAL
mac: DEFINES += HEADER_OPENAL_PREFIX

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

# plugin.qmltypes is used by Qt Creator for syntax highlighting and the QML code model.  It needs
# to be regenerated whenever the QML elements exported by the plugin change.  This cannot be done
# automatically at compile time because qmlplugindump does not support some QML features and it may
# not be possible when cross-compiling.
#
# To regenerate run 'make qmltypes' which will update the plugins.qmltypes file in the source
# directory.  Then review and commit the changes made to plugins.qmltypes.
#
# This will run the following command:
#     qmlplugindump <import name> <import version> <path to import plugin> > plugins.qmltypes
# e.g.:
#     qmlplugindump QtAudioEngine 5.0 imports/QtAudioEngine/libdeclarative_audioengine.so > plugins.qmltypes

load(resolve_target)
qmltypes.target = qmltypes
qmltypes.commands = $$[QT_INSTALL_BINS]/qmlplugindump QtAudioEngine 1.0 $$QMAKE_RESOLVED_TARGET > $$PWD/plugins.qmltypes
qmltypes.depends = $$QMAKE_RESOLVED_TARGET
QMAKE_EXTRA_TARGETS += qmltypes

# Tell qmake to create such makefile that qmldir, plugins.qmltypes and target
# (i.e. declarative_audioengine) are all copied to $$[QT_INSTALL_IMPORTS]/QtAudioEngine directory,

qmldir.files += $$PWD/qmldir $$PWD/plugins.qmltypes
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

INSTALLS += target qmldir
