find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(fftreal STATIC
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
)

target_compile_definitions(fftreal PRIVATE
    FFTREAL_LIBRARY
    LOG_ENGINE
    LOG_SPECTRUMANALYSER
    SPECTRUM_ANALYSER_SEPARATE_THREAD
    SUPERIMPOSE_PROGRESS_ON_WAVEFORM
)

target_link_libraries(fftreal PRIVATE Qt6::Core)

target_include_directories(fftreal INTERFACE .)
