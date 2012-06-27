SOURCES += main.cpp

win32: LIBS += -lOpenAL32
unix:!mac: LIBS += -lopenal
mac: LIBS += -framework OpenAL
mac: DEFINES += HEADER_OPENAL_PREFIX
