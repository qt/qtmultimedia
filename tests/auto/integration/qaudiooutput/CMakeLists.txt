# Generated from qaudiooutput.pro.

#####################################################################
## tst_qaudiooutput Test:
#####################################################################

qt_add_test(tst_qaudiooutput
    SOURCES
        tst_qaudiooutput.cpp
        wavheader.cpp wavheader.h
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::MultimediaPrivate
)
