requires(qtHaveModule(gui))

load(configure)
qtCompileTest(openal)
win32 {
    qtCompileTest(directshow)
    qtCompileTest(wmsdk)
    qtCompileTest(wmp)
    qtCompileTest(wmf)
    qtCompileTest(evr)
} else:mac {
    qtCompileTest(avfoundation)
} else:android {
    SDK_ROOT = $$(ANDROID_SDK_ROOT)
    isEmpty(SDK_ROOT): SDK_ROOT = $$DEFAULT_ANDROID_SDK_ROOT
    !exists($$SDK_ROOT/platforms/android-11/android.jar): error("QtMultimedia for Android requires API level 11")
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

