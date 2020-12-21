# Generated from qmlvideofilter_opencl.pro.

cmake_minimum_required(VERSION 3.14)
project(qmlvideofilter_opencl LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/qmlvideofilter_opencl")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Multimedia)

qt_add_executable(qmlvideofilter_opencl
    main.cpp
    rgbframehelper.h
)
set_target_properties(qmlvideofilter_opencl PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(qmlvideofilter_opencl PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Quick
)


# Resources:
set(qmlvideofilter_opencl_resource_files
    "main.qml"
)

qt6_add_resources(qmlvideofilter_opencl "qmlvideofilter_opencl"
    PREFIX
        "/"
    FILES
        ${qmlvideofilter_opencl_resource_files}
)

if(MACOS)
    target_link_libraries(qmlvideofilter_opencl PUBLIC
        "-framework OpenCL"
    )
endif()

if(UNIX AND NOT MACOS)
    target_link_libraries(qmlvideofilter_opencl PUBLIC
        OpenCL
    )
endif()

if(WIN32 AND NOT WINRT)
    target_link_libraries(qmlvideofilter_opencl PUBLIC
        OpenCL
        opengl32
    )
endif()

install(TARGETS qmlvideofilter_opencl
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
