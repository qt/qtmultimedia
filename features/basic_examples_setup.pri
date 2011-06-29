symbian: {
    vendorinfo = \
        "; Localised Vendor name" \
        "%{\"Nokia, Qt\"}" \
        " " \
        "; Unique Vendor name" \
        ":\"Nokia, Qt\"" \
        " "
    examples_deployment.pkg_prerules += vendorinfo
    DEPLOYMENT += examples_deployment
}

win32:contains(CONFIG_WIN32,build_all):Win32DebugAndRelease=yes
mac | contains(Win32DebugAndRelease,yes) {
    #due to different debug/release library names we have to comply with 
    #whatever Qt does
    !contains(QT_CONFIG,debug)|!contains(QT_CONFIG,release) {
        CONFIG -= debug_and_release debug release
        contains(QT_CONFIG,debug): CONFIG+=debug
        contains(QT_CONFIG,release): CONFIG+=release
    }
}

CONFIG(debug, debug|release) {
    SUBDIRPART=Debug
} else {
    SUBDIRPART=Release
}

OUTPUT_DIR = $$QT_MOBILITY_BUILD_TREE
MOC_DIR = $$OUTPUT_DIR/build/$$SUBDIRPART/$$TARGET/moc
RCC_DIR = $$OUTPUT_DIR/build/$$SUBDIRPART/$$TARGET/rcc
UI_DIR = $$OUTPUT_DIR/build/$$SUBDIRPART/$$TARGET/ui
OBJECTS_DIR = $$OUTPUT_DIR/build/$$SUBDIRPART/$$TARGET

# See common.pri for comments on why using QMAKE_FRAMEWORKPATH/QMAKE_LIBDIR
# rather than LIBS here.
mac:contains(QT_CONFIG,qt_framework) {
    QMAKE_FRAMEWORKPATH = $$OUTPUT_DIR/lib
}
QMAKE_LIBDIR = $$OUTPUT_DIR/lib

QMAKE_RPATHDIR+=$$QT_MOBILITY_LIB
INCLUDEPATH+= $$QT_MOBILITY_SOURCE_TREE/src/global

maemo6 {
    DEFINES+= Q_WS_MAEMO_6
    DEFINES+= QTM_EXAMPLES_SMALL_SCREEN
    DEFINES+= QTM_EXAMPLES_PREFER_LANDSCAPE
}
maemo5 {
    error(Maemo5/Freemantle not supported by QtMobility 1.2+ \(Not building any examples and demos\).)
    DEFINES+= Q_WS_MAEMO_5
    DEFINES+= QTM_EXAMPLES_SMALL_SCREEN
    DEFINES+= QTM_EXAMPLES_PREFER_LANDSCAPE
}
symbian {
    DEFINES+= QTM_EXAMPLES_SMALL_SCREEN
}
