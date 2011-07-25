load(qttest_p4)

QT += multimediakit-private
  
# TARGET = tst_qmediaobject
# CONFIG += testcase

include (../qmultimedia_common/mockrecorder.pri)

HEADERS+= tst_qmediaobject.h
SOURCES += main.cpp tst_qmediaobject.cpp

