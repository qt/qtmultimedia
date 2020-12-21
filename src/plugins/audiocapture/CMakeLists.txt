# Generated from audiocapture.pro.

#####################################################################
## AudioCaptureServicePlugin Plugin:
#####################################################################

qt_internal_add_plugin(AudioCaptureServicePlugin
    OUTPUT_NAME qtmedia_audioengine
    TYPE mediaservice
    SOURCES
        audiocaptureprobecontrol.cpp audiocaptureprobecontrol.h
        audiocaptureservice.cpp audiocaptureservice.h
        audiocaptureserviceplugin.cpp audiocaptureserviceplugin.h
        audiocontainercontrol.cpp audiocontainercontrol.h
        audioencodercontrol.cpp audioencodercontrol.h
        audioinputselector.cpp audioinputselector.h
        audiomediarecordercontrol.cpp audiomediarecordercontrol.h
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
)

#### Keys ignored in scope 1:.:.:audiocapture.pro:<TRUE>:
# OTHER_FILES = "audiocapture.json"
