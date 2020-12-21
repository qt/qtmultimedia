# Generated from qaudioinput.pro.

#####################################################################
## tst_qaudioinput Test:
#####################################################################

qt_add_test(tst_qaudioinput
    SOURCES
        tst_qaudioinput.cpp
        wavheader.cpp wavheader.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)
