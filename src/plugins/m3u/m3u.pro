load(qt_module)

TARGET = qtmultimediakit_m3u
QT += multimediakit-private
PLUGIN_TYPE=playlistformats

load(qt_plugin)
DESTDIR = $$QT.multimediakit.plugins/$${PLUGIN_TYPE}


HEADERS += qm3uhandler.h
SOURCES += main.cpp \
           qm3uhandler.cpp
symbian {
    TARGET.UID3 = 0x2002BFC7
    TARGET.CAPABILITY = ALL -TCB
    TARGET.EPOCALLOWDLLDATA = 1
    
    #make a sis package from plugin + stub (plugin)
    pluginDep.sources = $${TARGET}.dll
    pluginDep.path = $${QT_PLUGINS_BASE_DIR}/$${PLUGIN_TYPE}
    DEPLOYMENT += pluginDep
}
