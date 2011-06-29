load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qpaintervideosurface
# CONFIG += testcase

contains(QT_CONFIG, opengl): QT += opengl

SOURCES += tst_qpaintervideosurface.cpp

