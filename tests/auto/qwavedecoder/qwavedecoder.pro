TARGET = tst_qwavedecoder
HEADERS += ../../../src/multimedia/effects/qwavedecoder_p.h
SOURCES += tst_qwavedecoder.cpp \
           ../../../src/multimedia/effects/qwavedecoder_p.cpp

QT += multimedia-private testlib
CONFIG += no_private_qt_headers_warning testcase

data.path = .
data.sources = data

INSTALLS += data

