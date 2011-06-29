
win32:!win32-g++ {
    unixstyle = false
} else:win32-g++:isEmpty(QMAKE_SH) {
    unixstyle = false
} else {
    unixstyle = true
}

LINE_SEP=$$escape_expand(\\n\\t)
GENERATOR = $$[QT_INSTALL_BINS]/qhelpgenerator
QDOC = $$[QT_INSTALL_BINS]/qdoc3
MOBILITY_DOCUMENTATION = $$QDOC $${QT_MOBILITY_SOURCE_TREE}/doc/config/qtmobility.qdocconf $$LINE_SEP \
                         cd $${QT_MOBILITY_SOURCE_TREE} && \
                          $$GENERATOR doc/html/qtmobility.qhp -o doc/qch/qtmobility.qch

ONLINE_MOBILITY_DOCUMENTATION = $$QDOC $${QT_MOBILITY_SOURCE_TREE}/doc/config/qtmobility-online.qdocconf $$LINE_SEP \
                         cd $${QT_MOBILITY_SOURCE_TREE} && \
                          $$GENERATOR doc/html/qtmobility.qhp -o doc/qch/qtmobility.qch

contains(unixstyle, false):MOBILITY_DOCUMENTATION = $$replace(MOBILITY_DOCUMENTATION, "/", "\\")

# Build rules
qch_docs.commands = $$MOBILITY_DOCUMENTATION
qch_onlinedocs.commands = $$ONLINE_MOBILITY_DOCUMENTATION

docs.depends = qch_docs
onlinedocs.depends = qch_onlinedocs


QMAKE_EXTRA_TARGETS += qch_docs qch_onlinedocs docs onlinedocs
