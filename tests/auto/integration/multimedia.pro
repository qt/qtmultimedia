
TEMPLATE = subdirs
SUBDIRS += \
    qaudiodecoderbackend \
    qaudiodeviceinfo \
    qaudioinput \
    qaudiooutput \
    qdeclarativevideooutput \
    qdeclarativevideooutput_window \
    qmediaplayerbackend \
    qcamerabackend \
    qsoundeffect \
    qsound

!qtHaveModule(widgets): SUBDIRS -= qcamerabackend
