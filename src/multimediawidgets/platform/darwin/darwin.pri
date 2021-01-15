HEADERS += \
    $$PWD/avfvideowidgetcontrol_p.h \
    $$PWD/avfvideowidget_p.h

SOURCES += \
    $$PWD/avfvideowidgetcontrol.mm \
    $$PWD/avfvideowidget.mm

LIBS += -framework CoreFoundation \
        -framework Foundation \
        -framework QuartzCore \
        -framework CoreVideo \
        -framework Metal

QMAKE_USE += avfoundation
