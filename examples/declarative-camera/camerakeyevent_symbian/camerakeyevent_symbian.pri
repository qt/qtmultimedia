message("camerakeyevent_symbian: Including Symbian camera capture key event register methods")

HEADERS += $$PWD/camerakeyevent_symbian.h
SOURCES += $$PWD/camerakeyevent_symbian.cpp
INCLUDEPATH += $$PWD
LIBS *= -lcone -lws32
TARGET.CAPABILITY *= SwEvent
