INCLUDEPATH += $$PWD

contains(tunerlib_s60_enabled, yes) {

	LIBS += -ltunerutility
	DEFINES += TUNERLIBUSED
	INCLUDEPATH += $${EPOCROOT}epoc32/include/mmf/common

	HEADERS += $$PWD/s60radiotunercontrol_31.h
    SOURCES += $$PWD/s60radiotunercontrol_31.cpp
}

contains(radioutility_s60_enabled, yes) {
    LIBS += -lradio_utility
    DEFINES += RADIOUTILITYLIBUSED

    HEADERS += $$PWD/s60radiotunercontrol_since32.h
    SOURCES += $$PWD/s60radiotunercontrol_since32.cpp
}

contains(tunerlib_s60_enabled, yes)|contains(radioutility_s60_enabled, yes) {
    HEADERS += $$PWD/s60radiotunerservice.h
    SOURCES += $$PWD/s60radiotunerservice.cpp
}
