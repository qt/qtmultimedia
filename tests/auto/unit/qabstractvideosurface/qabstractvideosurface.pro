CONFIG += testcase
TARGET = tst_qabstractvideosurface

QT += core multimedia-private testlib
CONFIG += no_private_qt_headers_warning

SOURCES += tst_qabstractvideosurface.cpp

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
