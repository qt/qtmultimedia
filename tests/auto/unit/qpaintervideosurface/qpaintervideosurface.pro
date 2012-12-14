CONFIG += testcase
TARGET = tst_qpaintervideosurface

QT += multimedia-private multimediawidgets-private testlib
contains(QT_CONFIG, opengl) | contains(QT_CONFIG, opengles2) {
   QT += opengl
} else {
   DEFINES += QT_NO_OPENGL
}

contains(QT_CONFIG, opengl): QT += opengl

SOURCES += tst_qpaintervideosurface.cpp

QT+=widgets
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

win32:contains(QT_CONFIG, angle): CONFIG += insignificant_test # QTBUG-28542
