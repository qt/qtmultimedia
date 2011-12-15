TEMPLATE = subdirs

SUBDIRS += qmlvideo

# qmlvideofx requires QtOpenGL and ShaderEffectItem (added in Qt 4.7.4)
contains(QT_CONFIG, opengl) {
    lessThan(QT_MAJOR_VERSION, 5) {
        !lessThan(QT_MAJOR_VERSION, 4) {
            SUBDIRS += qmlvideofx
        }
    } else {
        SUBDIRS += qmlvideofx
    }
}

