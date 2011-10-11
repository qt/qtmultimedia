TARGET = tst_qwavedecoder
HEADERS += ../../../src/multimedia/effects/qwavedecoder_p.h
SOURCES += tst_qwavedecoder.cpp \
           ../../../src/multimedia/effects/qwavedecoder_p.cpp

QT += multimedia-private testlib network
CONFIG += no_private_qt_headers_warning testcase

data.files = data
data.path = $${OUT_PWD}

INSTALLS += data

