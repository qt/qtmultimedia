#####################################################################
## tst_qaudiosource Test:
#####################################################################

qt_internal_add_test(tst_qaudiosource
    SOURCES
        tst_qaudiosource.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)
