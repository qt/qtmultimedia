# Generated from fftreal.pro.

#####################################################################
## fftreal Generic Library:
#####################################################################

qt_add_cmake_library(fftreal
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/multimedia/spectrum"
    SOURCES
        Array.h Array.hpp
        DynArray.h DynArray.hpp
        FFTRealFixLen.h FFTRealFixLen.hpp
        FFTRealFixLenParam.h
        FFTRealPassDirect.h FFTRealPassDirect.hpp
        FFTRealPassInverse.h FFTRealPassInverse.hpp
        FFTRealSelect.h FFTRealSelect.hpp
        FFTRealUseTrigo.h FFTRealUseTrigo.hpp
        OscSinCos.h OscSinCos.hpp
        def.h
        fftreal_wrapper.cpp fftreal_wrapper.h
    DEFINES
        FFTREAL_LIBRARY
        LOG_ENGINE
        LOG_SPECTRUMANALYSER
        SPECTRUM_ANALYSER_SEPARATE_THREAD
        SUPERIMPOSE_PROGRESS_ON_WAVEFORM
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:fftreal.pro:<TRUE>:
# EXAMPLE_FILES = "bwins/fftreal.def" "eabi/fftreal.def" "readme.txt" "license.txt"
# INSTALLS = "target"
# TEMPLATE = "lib"
# target.path = "$$[QT_INSTALL_EXAMPLES]/multimedia/spectrum"

## Scopes:
#####################################################################

qt_extend_target(fftreal CONDITION static
    DEFINES
        DISABLE_FFT
)

#### Keys ignored in scope 4:.:.:fftreal.pro:else:
# DESTDIR = "../..$${spectrum_build_dir}"

qt_extend_target(fftreal CONDITION WIN32
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

#### Keys ignored in scope 10:.:../..:../../spectrum.pri:CONFIG(release,release OR debug):
# spectrum_build_dir = "/release"

#### Keys ignored in scope 11:.:../..:../../spectrum.pri:CONFIG(debug,release OR debug):
# spectrum_build_dir = "/debug"

# special case begin
if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/spectrum")

install(TARGETS fftreal
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
# special case end
