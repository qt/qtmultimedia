# Generated from app.pro.

cmake_minimum_required(VERSION 3.14)
project(spectrum LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/spectrum")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Multimedia)
find_package(Qt6 COMPONENTS Widgets)

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
    wavfile.cpp wavfile.h
)
set_target_properties(spectrum PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(spectrum PUBLIC
    ../3rdparty/fftreal
    ../../shared
)

target_compile_definitions(spectrum PUBLIC
    LOG_ENGINE
    LOG_SPECTRUMANALYSER
    SPECTRUM_ANALYSER_SEPARATE_THREAD
    SUPERIMPOSE_PROGRESS_ON_WAVEFORM
)

target_link_libraries(spectrum PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Widgets
)


# Resources:
set(spectrum_resource_files
    "images/record.png"
    "images/settings.png"
)

qt6_add_resources(spectrum "spectrum"
    PREFIX
        "/"
    FILES
        ${spectrum_resource_files}
)

if(MACOS AND BUILD_SHARED_LIBS) # special case
    target_link_libraries(spectrum PUBLIC
        fftreal # special case
        # Remove: F
    )
endif()


if(BUILD_SHARED_LIBS AND NOT MACOS) # special case
    target_link_libraries(spectrum PUBLIC
        # Remove: L..
        fftreal
    )
endif()

if(static)
    target_compile_definitions(spectrum PUBLIC
        DISABLE_FFT
    )
endif()

if(WIN32)
    target_compile_definitions(spectrum PUBLIC
        _CRT_SECURE_NO_WARNINGS
    )
endif()

install(TARGETS spectrum
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
