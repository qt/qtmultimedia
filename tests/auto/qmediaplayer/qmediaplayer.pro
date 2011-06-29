load(qttest_p4)

QT += network multimediakit-private

# TARGET = tst_qmediaplayer
# CONFIG += testcase

symbian {
  TARGET.CAPABILITY = NetworkServices
  testFiles.sources = testfiles/*
  testFiles.path = /Data/testfiles
  DEPLOYMENT += testFiles
  contains(openmaxal_symbian_enabled, no) {
    DEFINES += HAS_OPENMAXAL_MEDIAPLAY_BACKEND
    HEADERS += tst_qmediaplayer_xa.h
    SOURCES += tst_qmediaplayer_xa.cpp
  } else {
    HEADERS += tst_qmediaplayer_s60.h
    SOURCES += tst_qmediaplayer_s60.cpp
  }
}

HEADERS += tst_qmediaplayer.h
SOURCES += main.cpp tst_qmediaplayer.cpp

