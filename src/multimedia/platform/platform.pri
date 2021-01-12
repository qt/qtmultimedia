qtConfig(gstreamer):include(gstreamer/gstreamer.pri)
qtConfig(pulseaudio): include(pulseaudio/pulseaudio.pri)
qtConfig(alsa): include(alsa/alsa.pri)

android: include(opensles/opensles.pri)
win32: include(wasapi/wasapi.pri)
darwin:!watchos: include(coreaudio/coreaudio.pri)
qnx: include(qnx/qnx.pri)

