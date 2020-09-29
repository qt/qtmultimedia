TARGET = Qt$${QT_MAJOR_VERSION}AndroidMultimedia

load(qt_build_paths)
CONFIG += java
DESTDIR = $$MODULE_BASE_OUTDIR/jar

JAVACLASSPATH += $$PWD/src

JAVASOURCES += $$PWD/src/org/qtproject/qt/android/multimedia/QtAndroidMediaPlayer.java \
               $$PWD/src/org/qtproject/qt/android/multimedia/QtCameraListener.java \
               $$PWD/src/org/qtproject/qt/android/multimedia/QtSurfaceTextureListener.java \
               $$PWD/src/org/qtproject/qt/android/multimedia/QtSurfaceTextureHolder.java \
               $$PWD/src/org/qtproject/qt/android/multimedia/QtMultimediaUtils.java \
               $$PWD/src/org/qtproject/qt/android/multimedia/QtMediaRecorderListener.java \
               $$PWD/src/org/qtproject/qt/android/multimedia/QtSurfaceHolderCallback.java

# install
target.path = $$[QT_INSTALL_PREFIX]/jar
INSTALLS += target

OTHER_FILES += $$JAVASOURCES
