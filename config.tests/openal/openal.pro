TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

SOURCES += main.cpp

win32 {
    LIBS += -lOpenAL32
}else {
    LIBS += -lopenal
}
