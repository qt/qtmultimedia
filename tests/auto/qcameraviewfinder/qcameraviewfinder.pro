load(qttest_p4)

QT += multimediakit-private multimediakitwidgets-private

include (../qmultimedia_common/mock.pri)
include (../qmultimedia_common/mockcamera.pri)

SOURCES += tst_qcameraviewfinder.cpp
