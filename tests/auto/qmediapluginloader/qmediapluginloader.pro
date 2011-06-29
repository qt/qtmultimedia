load(qttest_p4)

QT += multimediakit-private

# TARGET = tst_qmediapluginloader
# CONFIG += testcase

SOURCES += tst_qmediapluginloader.cpp

wince* {
    PLUGIN_DEPLOY.sources = $$OUTPUT_DIR/plugins/mediaservice/*.dll
    PLUGIN_DEPLOY.path = mediaservice
    DEPLOYMENT += PLUGIN_DEPLOY
}

