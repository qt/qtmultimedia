include(spectrum.pri)

TEMPLATE = app

TARGET = spectrum

QT       += multimedia widgets

SOURCES  += main.cpp \
            engine.cpp \
            frequencyspectrum.cpp \
            levelmeter.cpp \
            mainwidget.cpp \
            progressbar.cpp \
            settingsdialog.cpp \
            spectrograph.cpp \
            spectrumanalyser.cpp \
            tonegenerator.cpp \
            tonegeneratordialog.cpp \
            utils.cpp \
            waveform.cpp \

HEADERS  += engine.h \
            frequencyspectrum.h \
            levelmeter.h \
            mainwidget.h \
            progressbar.h \
            settingsdialog.h \
            spectrograph.h \
            spectrum.h \
            spectrumanalyser.h \
            tonegenerator.h \
            tonegeneratordialog.h \
            utils.h \
            waveform.h \

fftreal_dir = 3rdparty/fftreal

INCLUDEPATH += $${fftreal_dir}

RESOURCES = spectrum.qrc

LIBS += -L./$${spectrum_build_dir}
LIBS += -lfftreal

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/spectrum
INSTALLS += target

CONFIG += install_ok  # Do not cargo-cult this!

# Deployment

DESTDIR = ./$${spectrum_build_dir}
include(../shared/shared.pri)
