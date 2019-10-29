QT += quick multimedia multimediawidgets
CONFIG += c++11

SOURCES += main.cpp

RESOURCES += qml.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/video/android/gstreamer
INSTALLS += target

GSTREAMER_ROOT_ANDROID = $$(GSTREAMER_ROOT_ANDROID)
isEmpty(GSTREAMER_ROOT_ANDROID): error("GSTREAMER_ROOT_ANDROID is empty")

INCLUDEPATH += $(GSTREAMER_ROOT_ANDROID)/armv7/include/ $(GSTREAMER_ROOT_ANDROID)/armv7/include/gstreamer-1.0 $(GSTREAMER_ROOT_ANDROID)/armv7/include/glib-2.0 $(GSTREAMER_ROOT_ANDROID)/armv7/lib/glib-2.0/include

QT+=multimediagsttools-private
LIBS += -L$(GSTREAMER_ROOT_ANDROID)/armv7/lib/ -L$(GSTREAMER_ROOT_ANDROID)/armv7/lib/gstreamer-1.0  \
-lgstcoreelements -lgstplayback -lgstvideotestsrc -lgstaudioconvert -lgstvideoconvert -lgstautodetect -lgsttypefindfunctions \
#PLUGINS codecs
-lgstvorbis  -lvorbis  -lgstivorbisdec -lvorbisenc -lvorbisfile -lgstsubparse  -lgstaudioparsers  \
-lgstgio -lgstapp -lgstisomp4 -lgstavi -lgstogg  -lgstwavenc -lgstwavpack -lgstwavparse -lgsttheora -lgstmpg123 -lgstx264 -lgstlibav \
-lgsttcp -lgstsoup -logg  -ltheora -lmpg123  -lx264  -lavfilter  -lavformat   -lavcodec -lavutil -lbz2 -lswresample \
-lsoup-2.4 -lgio-2.0  -lgstrtp-1.0 -lgstriff-1.0 -lgstnet-1.0
