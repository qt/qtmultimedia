load(configure)
qtCompileTest(openal)
win32 {
    qtCompileTest(directshow)
    qtCompileTest(wmsdk)
    qtCompileTest(wmp)
    qtCompileTest(wmf)
    qtCompileTest(evr)
} else {
    qtCompileTest(alsa)
    qtCompileTest(pulseaudio)
    qtCompileTest(gstreamer) {
        qtCompileTest(gstreamer_photography)
        qtCompileTest(gstreamer_encodingprofiles)
        qtCompileTest(gstreamer_appsrc)
    }
    qtCompileTest(resourcepolicy)
    qtCompileTest(xvideo)
}

load(qt_parts)

# for make docs:
include(src/multimedia/doc/qtmultimedia_doc.pri)
