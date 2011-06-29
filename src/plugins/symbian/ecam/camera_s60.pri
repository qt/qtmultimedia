INCLUDEPATH += $$PWD

include (../videooutput/videooutput.pri)

# Camera Service
DEFINES += QMEDIA_SYMBIAN_CAMERA

# S60 3.1 platform
contains(S60_VERSION, 3.1) {
    DEFINES += S60_31_PLATFORM
    DEFINES *= S60_3X_PLATFORM
}

# S60 3.2 platform
contains(S60_VERSION, 3.2) {
    DEFINES += S60_32_PLATFORM
    DEFINES *= S60_3X_PLATFORM
}

# S60 5.0 platform
!contains(DEFINES, S60_31_PLATFORM) {
    !contains(DEFINES, S60_32_PLATFORM) {
        !contains(DEFINES, SYMBIAN_3_PLATFORM) {
            DEFINES += S60_50_PLATFORM
        }
    }
}

# Symbian 3 platform
contains(DEFINES, VIDEOOUTPUT_GRAPHICS_SURFACES) {
    DEFINES += SYMBIAN_3_PLATFORM
}

# AutoFocusing (CamAutoFocus) from ForumNokia example
contains(symbian_camera_camautofocus_enabled, yes) {
    exists($${EPOCROOT}epoc32\\include\\CCamAutoFocus.h) {
        message ("CameraBE: Using S60 3.1 autofocusing")
        MMP_RULES += \
            "$${LITERAL_HASH}ifdef WINSCW" \
            "LIBRARY camautofocus.lib" \
            "$${LITERAL_HASH}else" \
            "STATICLIBRARY camautofocus_s.lib" \
            "$${LITERAL_HASH}endif // WINS" \
            "MACRO S60_CAM_AUTOFOCUS_SUPPORT"
    }
}

# ECam AdvancedSettings
contains(symbian_camera_ecamadvsettings_enabled, yes) {
    exists($${EPOCROOT}epoc32\\include\\ecamadvancedsettings.h) {
        MMP_RULES += \
            "$${LITERAL_HASH}ifndef WINSCW" \
            "LIBRARY ecamadvsettings.lib" \
            "MACRO USE_S60_32_ECAM_ADVANCED_SETTINGS_HEADER" \
            "$${LITERAL_HASH}endif"
        message("CameraBE: Using from S60 3.2 CCameraAdvancedSettings header")
    }
    exists($${EPOCROOT}epoc32\\include\\ecamadvsettings.h) {
        symbian:LIBS += -lecamadvsettings
        DEFINES += USE_S60_50_ECAM_ADVANCED_SETTINGS_HEADER
        message("CameraBE: Using CCameraAdvancedSettings header from S60 5.0 or later")
    }
}

# DevVideo API Check (Requires both, DevVideoPlay and DevVideoRecord plugins):
# DevVideoConstants has been problematic since not being included in SDK plugins
# For S60 5.0 this has changed with plugin extension 1.1
# But for S60 3.2 this is still a problem
contains(symbian_camera_devvideorecord_enabled, yes) {
    exists($${EPOCROOT}epoc32\\include\\mmf\\devvideo\\devvideorecord.h) {
        exists($${EPOCROOT}epoc32\\include\\mmf\\devvideo\\devvideobase.h) {
            exists($${EPOCROOT}epoc32\\include\\mmf\\devvideo\\devvideoconstants.h) {
                symbian:LIBS += -ldevvideo
                DEFINES += S60_DEVVIDEO_RECORDING_SUPPORTED
                message("CameraBE: Devvideo API supported")
            }
        }
    }
}

# ECam Snapshot API:
contains(symbian_camera_snapshot_enabled, yes) {
    exists($${EPOCROOT}epoc32\\include\\platform\\ecam\\camerasnapshot.h) {
        DEFINES += ECAM_PREVIEW_API
        message("CameraBE: Using CCameraSnapshot API")
        symbian:LIBS += -lecamsnapshot
    } else {
        message("CameraBE: Using custom snapshot proving methods")
    }
} else {
    message("CameraBE: Using custom snapshot proving methods")
}

# Libraries:
symbian:LIBS += -lfbscli \
        -lmediaclientvideo \
        -lecam \
        -lbafl \
        -lPlatformEnv \
        -lcharconv \
        -lconvnames \
        -lgb2312_shared \
        -ljisx0201 \
        -ljisx0208 \
        -lmmfcontrollerframework \
        -lfbscli \
        -lefsrv \
        -lcone \
        -lws32 \
        -limageconversion

# Source:
HEADERS += $$PWD/s60cameraconstants.h \
    $$PWD/s60cameralockscontrol.h \
    $$PWD/s60camerafocuscontrol.h \
    $$PWD/s60cameraexposurecontrol.h \
    $$PWD/s60cameraflashcontrol.h \
    $$PWD/s60cameracontrol.h \
    $$PWD/s60mediarecordercontrol.h \
    $$PWD/s60videocapturesession.h \
    $$PWD/s60imagecapturesession.h \
    $$PWD/s60mediacontainercontrol.h \
    $$PWD/s60videoencodercontrol.h \
    $$PWD/s60audioencodercontrol.h \
    $$PWD/s60cameraservice.h \
    $$PWD/s60cameraimageprocessingcontrol.h \
    $$PWD/s60cameraimagecapturecontrol.h \
    $$PWD/s60videodevicecontrol.h \
    $$PWD/s60imageencodercontrol.h \
    $$PWD/s60camerasettings.h \
    $$PWD/s60cameraengine.h \
    $$PWD/s60cameraviewfinderengine.h \
    $$PWD/s60cameraengineobserver.h \
    $$PWD/s60videorenderercontrol.h

SOURCES += $$PWD/s60cameralockscontrol.cpp \
    $$PWD/s60camerafocuscontrol.cpp \
    $$PWD/s60cameraexposurecontrol.cpp \
    $$PWD/s60cameraflashcontrol.cpp \
    $$PWD/s60cameracontrol.cpp \
    $$PWD/s60mediarecordercontrol.cpp \
    $$PWD/s60videocapturesession.cpp \
    $$PWD/s60imagecapturesession.cpp \
    $$PWD/s60mediacontainercontrol.cpp \
    $$PWD/s60videoencodercontrol.cpp \
    $$PWD/s60audioencodercontrol.cpp \
    $$PWD/s60cameraservice.cpp \
    $$PWD/s60cameraimageprocessingcontrol.cpp \
    $$PWD/s60cameraimagecapturecontrol.cpp \
    $$PWD/s60videodevicecontrol.cpp \
    $$PWD/s60imageencodercontrol.cpp \
    $$PWD/s60camerasettings.cpp \
    $$PWD/s60cameraengine.cpp \
    $$PWD/s60cameraviewfinderengine.cpp \
    $$PWD/s60videorenderercontrol.cpp

# End of file
