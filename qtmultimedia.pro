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

module_qtmultimedia_docsnippets.subdir = doc
module_qtmultimedia_docsnippets.target = sub-doc
module_qtmultimedia_docsnippets.depends = sub_src
module_qtmultimedia_docsnippets.CONFIG = no_default_install

SUBDIRS += module_qtmultimedia_docsnippets

# for make docs:
include(doc/config/qtmultimedia_doc.pri)
