
TEMPLATE = subdirs
SUBDIRS += \
    qaudiodecoderbackend \
    qaudiodeviceinfo \
    qaudioinput \
    qaudiooutput \
    qmediaplayerbackend \
    qcamerabackend \
    qsoundeffect \

qtHaveModule(quick) {
    SUBDIRS += \
        qdeclarativevideooutput \
        qdeclarativevideooutput_window
}

!qtHaveModule(widgets): SUBDIRS -= qcamerabackend
