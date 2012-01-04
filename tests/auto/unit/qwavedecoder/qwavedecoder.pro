TARGET = tst_qwavedecoder
HEADERS += $$QT.multimedia.sources/audio/qwavedecoder_p.h
SOURCES += tst_qwavedecoder.cpp \
           $$QT.multimedia.sources/audio/qwavedecoder_p.cpp

QT += multimedia-private testlib network
CONFIG += no_private_qt_headers_warning testcase

data.files = data/*
data.path = data
DEPLOYMENT += data

