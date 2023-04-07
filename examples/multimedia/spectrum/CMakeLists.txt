# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(spectrum LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/spectrum")

find_package(Qt6 REQUIRED COMPONENTS Multimedia Widgets)

add_subdirectory(3rdparty/fftreal)

qt_add_executable(spectrum
    engine.cpp engine.h
    frequencyspectrum.cpp frequencyspectrum.h
    levelmeter.cpp levelmeter.h
    main.cpp
    mainwidget.cpp mainwidget.h
    progressbar.cpp progressbar.h
    settingsdialog.cpp settingsdialog.h
    spectrograph.cpp spectrograph.h
    spectrum.h
    spectrumanalyser.cpp spectrumanalyser.h
    tonegenerator.cpp tonegenerator.h
    tonegeneratordialog.cpp tonegeneratordialog.h
    utils.cpp utils.h
    waveform.cpp waveform.h
)

set_target_properties(spectrum PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
)

target_compile_definitions(spectrum PRIVATE
    LOG_ENGINE
    LOG_SPECTRUMANALYSER
    SPECTRUM_ANALYSER_SEPARATE_THREAD
    SUPERIMPOSE_PROGRESS_ON_WAVEFORM
)

target_link_libraries(spectrum PRIVATE
    Qt6::Multimedia
    Qt6::Widgets
    fftreal
)

qt_add_resources(spectrum "spectrum"
    PREFIX
        "/"
    FILES
        "images/record.png"
        "images/settings.png"
)

install(TARGETS spectrum
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
