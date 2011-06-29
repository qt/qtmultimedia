include(../features/basic_examples_setup.pri)

!plugin {
    target.path=$$QT_MOBILITY_DEMOS
} else {
    target.path = $${QT_MOBILITY_PLUGINS}/$${PLUGIN_TYPE}
}
INSTALLS += target


