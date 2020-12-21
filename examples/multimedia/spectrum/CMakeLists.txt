# Generated from spectrum.pro.

add_subdirectory(app)
if(NOT DEFINES___contains___DISABLE_FFT)
    add_subdirectory(3rdparty/fftreal)
endif()
