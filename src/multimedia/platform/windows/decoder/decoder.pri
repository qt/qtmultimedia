INCLUDEPATH += $$PWD

LIBS += -lmfreadwrite -lwmcodecdspuuid
QMAKE_USE += wmf

HEADERS += \
    $$PWD/mfdecodersourcereader_p.h \
    $$PWD/mfaudiodecodercontrol_p.h

SOURCES += \
    $$PWD/mfdecodersourcereader.cpp \
    $$PWD/mfaudiodecodercontrol.cpp
