TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

requires(unix)

SOURCES = alsatest.cpp

CONFIG -= qt dylib
mac:CONFIG -= app_bundle

LIBS+=-lasound

