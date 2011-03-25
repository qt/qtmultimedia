TEMPLATE=subdirs
SUBDIRS=\
    qabstractvideobuffer \
    qabstractvideosurface \
    qaudiodeviceinfo \
    qaudioformat \
    qaudioinput \
    qaudiooutput \
    qvideoframe \
    qvideosurfaceformat
!cross_compile:                             SUBDIRS += host.pro
