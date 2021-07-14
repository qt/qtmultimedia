include(spectrum.pri)

TEMPLATE = subdirs

SUBDIRS += 3rdparty/fftreal

app.file = app.pro
app.depends = 3rdparty/fftreal
SUBDIRS += app

TARGET = spectrum

EXAMPLE_FILES += \
    README.txt \
    TODO.txt
