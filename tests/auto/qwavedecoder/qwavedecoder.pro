load(qttest_p4)

HEADERS += ../../../src/multimedia/effects/qwavedecoder_p.h
SOURCES += tst_qwavedecoder.cpp \
           ../../../src/multimedia/effects/qwavedecoder_p.cpp

QT += multimedia-private
CONFIG += no_private_qt_headers_warning

data.path = .
data.sources = data

INSTALLS += data

