include(spectrum.pri)

TEMPLATE = subdirs

# Ensure that library is built before application
CONFIG  += ordered
QT += widgets

!contains(DEFINES, DISABLE_FFT): SUBDIRS += 3rdparty/fftreal
SUBDIRS += app

TARGET = spectrum

