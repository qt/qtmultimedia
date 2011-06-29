load(qttest_p4)

QT += multimediakit-private
  
# TARGET = tst_qmediaobject
# CONFIG += testcase

symbian {
  HEADERS += tst_qmediaobject_xa.h
  SOURCES += tst_qmediaobject_xa.cpp
  TARGET.CAPABILITY = ALL -TCB

  testFiles.sources = testfiles/*
  testFiles.path = /Data/testfiles
  DEPLOYMENT += testFiles
  HEADERS += tst_qmediaobject_mmf.h
  SOURCES += tst_qmediaobject_mmf.cpp
  TARGET.CAPABILITY = ALL -TCB

  testFiles.sources = testfiles/*
  testFiles.path = /Data/testfiles
  DEPLOYMENT += testFiles
}

HEADERS+= tst_qmediaobject.h
SOURCES += main.cpp tst_qmediaobject.cpp

