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
} else:android:!android-no-sdk {
    SDK_ROOT = $$(ANDROID_SDK_ROOT)
    isEmpty(SDK_ROOT): SDK_ROOT = $$DEFAULT_ANDROID_SDK_ROOT
    !exists($$SDK_ROOT/platforms/android-11/android.jar): error("QtMultimedia for Android requires API level 11")
} else:qnx {
    qtCompileTest(mmrenderer)
} else {
    qtCompileTest(alsa)
    qtCompileTest(pulseaudio)
    qtCompileTest(gstreamer) {
        qtCompileTest(gstreamer_photography)
        qtCompileTest(gstreamer_encodingprofiles)
        qtCompileTest(gstreamer_appsrc)
    }
    qtCompileTest(resourcepolicy)
    qtCompileTest(gpu_vivante)
}

load(qt_parts)

