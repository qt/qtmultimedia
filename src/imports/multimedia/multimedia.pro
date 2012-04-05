TARGET = declarative_multimedia
TARGETPATH = QtMultimedia

include(../qimportbase.pri)

QT += qml quick network multimedia-private

DESTDIR = $$QT.multimedia.imports/$$TARGETPATH
target.path = $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

LIBS += -lQtMultimediaQuick_p

HEADERS += \
        qdeclarativeaudio_p.h \
        qdeclarativemediametadata_p.h \
        qdeclarativevideooutput_p.h \
        qdeclarativevideooutput_backend_p.h \
        qdeclarativevideooutput_render_p.h \
        qdeclarativevideooutput_window_p.h \
        qsgvideonode_i420.h \
        qsgvideonode_rgb.h \
        qdeclarativeradio_p.h \
        qdeclarativeradiodata_p.h \
        qdeclarativecamera_p.h \
        qdeclarativecameracapture_p.h \
        qdeclarativecamerarecorder_p.h \
        qdeclarativecameraexposure_p.h \
        qdeclarativecameraflash_p.h \
        qdeclarativecamerafocus_p.h \
        qdeclarativecameraimageprocessing_p.h \
        qdeclarativecamerapreviewprovider_p.h \
        qdeclarativetorch_p.h \
        qdeclarativeaudio_p_4.h \
        qdeclarativemediabase_p_4.h \
        qdeclarativemediametadata_p_4.h

SOURCES += \
        multimedia.cpp \
        qdeclarativeaudio.cpp \
        qdeclarativevideooutput.cpp \
        qdeclarativevideooutput_render.cpp \
        qdeclarativevideooutput_window.cpp \
        qsgvideonode_i420.cpp \
        qsgvideonode_rgb.cpp \
        qdeclarativeradio.cpp \
        qdeclarativeradiodata.cpp \
        qdeclarativecamera.cpp \
        qdeclarativecameracapture.cpp \
        qdeclarativecamerarecorder.cpp \
        qdeclarativecameraexposure.cpp \
        qdeclarativecameraflash.cpp \
        qdeclarativecamerafocus.cpp \
        qdeclarativecameraimageprocessing.cpp \
        qdeclarativecamerapreviewprovider.cpp \
        qdeclarativetorch.cpp \
        qdeclarativemediabase_4.cpp \
        qdeclarativeaudio_4.cpp

OTHER_FILES += \
    Video_4.qml \
    Video.qml

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
#     qmlplugindump QtMultimedia 5.0 imports/QtMultimedia/libdeclarative_multimedia.so > plugins.qmltypes

load(resolve_target)
qmltypes.target = qmltypes
qmltypes.commands = $$[QT_INSTALL_BINS]/qmlplugindump QtMultimedia 5.0 $$QMAKE_RESOLVED_TARGET > $$PWD/plugins.qmltypes
qmltypes.depends = $$QMAKE_RESOLVED_TARGET
QMAKE_EXTRA_TARGETS += qmltypes

# Tell qmake to create such makefile that qmldir, plugins.qmltypes and target
# (i.e. declarative_multimedia) are all copied to $$[QT_INSTALL_IMPORTS]/QtMultimedia directory,

qmldir.files += $$PWD/qmldir $$PWD/plugins.qmltypes $$PWD/Video.qml $$PWD/Video_4.qml
qmldir.path +=  $$[QT_INSTALL_IMPORTS]/$$TARGETPATH

# another copy of the qmldir file so the old import works
OLDQMLDIRFILE = $${_PRO_FILE_PWD_}/qmldir.qtmultimediakit
oldcopy2build.input = OLDQMLDIRFILE
oldcopy2build.output = $$QT.multimedia.imports/Qt/multimediakit/qmldir
!contains(TEMPLATE_PREFIX, vc):oldcopy2build.variable_out = PRE_TARGETDEPS
oldcopy2build.commands = $$QMAKE_COPY ${QMAKE_FILE_IN} ${QMAKE_FILE_OUT}
oldcopy2build.name = COPY ${QMAKE_FILE_IN}
oldcopy2build.CONFIG += no_link
# `clean' should leave the build in a runnable state, which means it shouldn't delete qmldir
oldcopy2build.CONFIG += no_clean
QMAKE_EXTRA_COMPILERS += oldcopy2build

INSTALLS += target qmldir
