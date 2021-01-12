LIBS += -framework CoreFoundation \
        -framework Foundation \
        -framework AudioToolbox \
        -framework CoreAudio \
        -framework QuartzCore \
        -framework CoreMedia \
        -framework CoreVideo \
        -framework QuartzCore \
        -framework Metal
osx:LIBS += -framework AppKit \
            -framework AudioUnit
ios:LIBS += -framework CoreGraphics \
            -framework CoreVideo

QMAKE_USE += avfoundation

include(mediaplayer/mediaplayer.pri)
!tvos:include(camera/camera.pri)
