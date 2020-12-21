# Generated from camera.pro.

#####################################################################
## AVFServicePlugin Plugin:
#####################################################################

qt_internal_add_plugin(AVFServicePlugin
    OUTPUT_NAME qavfcamera
    TYPE mediaservice
    SOURCES
        avfaudioencodersettingscontrol.h avfaudioencodersettingscontrol.mm
        avfaudioinputselectorcontrol.h avfaudioinputselectorcontrol.mm
        avfcameracontrol.h avfcameracontrol.mm
        avfcameradebug.h
        avfcameradevicecontrol.h avfcameradevicecontrol.mm
        avfcameraexposurecontrol.h avfcameraexposurecontrol.mm
        avfcameraflashcontrol.h avfcameraflashcontrol.mm
        avfcamerafocuscontrol.h avfcamerafocuscontrol.mm
        avfcamerainfocontrol.h avfcamerainfocontrol.mm
        avfcamerametadatacontrol.h avfcamerametadatacontrol.mm
        avfcamerarenderercontrol.h avfcamerarenderercontrol.mm
        avfcameraservice.h avfcameraservice.mm
        avfcameraserviceplugin.h avfcameraserviceplugin.mm
        avfcamerasession.h avfcamerasession.mm
        avfcamerautility.h avfcamerautility.mm
        avfcameraviewfindersettingscontrol.h avfcameraviewfindersettingscontrol.mm
        avfcamerawindowcontrol.h avfcamerawindowcontrol.mm
        avfcapturedestinationcontrol.h avfcapturedestinationcontrol.mm
        avfimagecapturecontrol.h avfimagecapturecontrol.mm
        avfimageencodercontrol.h avfimageencodercontrol.mm
        avfmediacontainercontrol.h avfmediacontainercontrol.mm
        avfmediavideoprobecontrol.h avfmediavideoprobecontrol.mm
        avfstoragelocation.h avfstoragelocation.mm
        avfvideoencodersettingscontrol.h avfvideoencodersettingscontrol.mm
    PUBLIC_LIBRARIES
        ${FWAudioToolbox}
        ${FWCoreAudio}
        ${FWCoreFoundation}
        ${FWCoreMedia}
        ${FWFoundation}
        ${FWQuartzCore}
        Qt::Core
        Qt::Gui
        Qt::MultimediaPrivate
        Qt::Network
        avfoundation
)

#### Keys ignored in scope 1:.:.:camera.pro:<TRUE>:
# OTHER_FILES = "avfcamera.json"

## Scopes:
#####################################################################

qt_internal_extend_target(AVFServicePlugin CONDITION MACOS
    SOURCES
        avfmediarecordercontrol.h avfmediarecordercontrol.mm
    PUBLIC_LIBRARIES
        ${FWAppKit}
        ${FWAudioUnit}
)

qt_internal_extend_target(AVFServicePlugin CONDITION IOS
    SOURCES
        avfcamerazoomcontrol.h avfcamerazoomcontrol.mm
        avfmediaassetwriter.h avfmediaassetwriter.mm
        avfmediarecordercontrol_ios.h avfmediarecordercontrol_ios.mm
    PUBLIC_LIBRARIES
        ${FWCoreGraphics}
        ${FWCoreVideo}
)
