# Generated from spectrum.pro.

add_subdirectory(app)
if(BUILD_SHARED_LIBS) # special case
    add_subdirectory(3rdparty/fftreal)
endif()
