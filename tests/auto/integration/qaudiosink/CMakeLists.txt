#####################################################################
## tst_qaudiosink Test:
#####################################################################

qt_internal_add_test(tst_qaudiosink
    SOURCES
        tst_qaudiosink.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)
