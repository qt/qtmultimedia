load(qt_module)

TARGET = qtmedia_fakeradio
QT += multimedia-private
PLUGIN_TYPE = mediaservice

load(qt_plugin)
DESTDIR = $$QT.multimedia.plugins/$${PLUGIN_TYPE}

HEADERS += \
  fakeradioserviceplugin.h \
  fakeradioservice.h \
  fakeradiotunercontrol.h \
  fakeradiodatacontrol.h

SOURCES += \
  fakeradioserviceplugin.cpp \
  fakeradioservice.cpp \
  fakeradiotunercontrol.cpp \
  fakeradiodatacontrol.cpp

target.path += $$[QT_INSTALL_PLUGINS]/$${PLUGIN_TYPE}
INSTALLS += target

