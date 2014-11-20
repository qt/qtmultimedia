requires(qtHaveModule(gui))

load(configure)
qtCompileTest(openal)
win32 {
    qtCompileTest(directshow) {
        qtCompileTest(wshellitem)
    }
    qtCompileTest(wmsdk)
    qtCompileTest(wmp)
    contains(QT_CONFIG, wmf-backend): qtCompileTest(wmf)
    qtCompileTest(evr)
} else:mac {
    qtCompileTest(avfoundation)
} else:qnx {
    qtCompileTest(mmrenderer)
} else {
    qtCompileTest(alsa)
    qtCompileTest(pulseaudio)
    !done_config_gstreamer {
        gstver=0.10
        !isEmpty(GST_VERSION): gstver=$$GST_VERSION
        cache(GST_VERSION, set, gstver);
        qtCompileTest(gstreamer) {
            qtCompileTest(gstreamer_photography)
            qtCompileTest(gstreamer_encodingprofiles)
            qtCompileTest(gstreamer_appsrc)
            qtCompileTest(linux_v4l)
        } else {
            gstver=1.0
            cache(GST_VERSION, set, gstver);
            # Force a re-run of the test
            CONFIG -= done_config_gstreamer
            qtCompileTest(gstreamer) {
                qtCompileTest(gstreamer_photography)
                qtCompileTest(gstreamer_encodingprofiles)
                qtCompileTest(gstreamer_appsrc)
                qtCompileTest(linux_v4l)
            }
        }
    }
    qtCompileTest(resourcepolicy)
    qtCompileTest(gpu_vivante)
}

load(qt_parts)

