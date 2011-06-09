load(qttest_p4)

QT += multimediakit-private
contains(QT_CONFIG, opengl) | contains(QT_CONFIG, opengles2): !symbian {
   QT += opengl
} else {
   DEFINES += QT_NO_OPENGL
}

# TARGET = tst_qpaintervideosurface
# CONFIG += testcase

contains(QT_CONFIG, opengl): QT += opengl

SOURCES += tst_qpaintervideosurface.cpp

