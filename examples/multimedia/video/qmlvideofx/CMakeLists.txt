# Generated from qmlvideofx.pro.

cmake_minimum_required(VERSION 3.14)
project(qmlvideofx LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/qmlvideofx")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Quick)
find_package(Qt6 COMPONENTS Multimedia)
find_package(Qt6 COMPONENTS Qml)

qt_add_executable(qmlvideofx
    ../snippets/frequencymonitor/frequencymonitor.cpp ../snippets/frequencymonitor/frequencymonitor.h
    ../snippets/frequencymonitor/frequencymonitordeclarative.cpp
    ../snippets/performancemonitor/performancemonitor.cpp ../snippets/performancemonitor/performancemonitor.h
    ../snippets/performancemonitor/performancemonitordeclarative.cpp ../snippets/performancemonitor/performancemonitordeclarative.h
    filereader.cpp filereader.h
    main.cpp
    trace.h
)
set_target_properties(qmlvideofx PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(qmlvideofx PUBLIC
    ../snippets/frequencymonitor
    ../snippets/performancemonitor
)

target_compile_definitions(qmlvideofx PUBLIC
    FREQUENCYMONITOR_SUPPORT
    PERFORMANCEMONITOR_SUPPORT
)

target_link_libraries(qmlvideofx PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Multimedia
    Qt::Qml
    Qt::Quick
)


# Resources:
set(qmlvideofx_resource_files
    "images/Dropdown_arrows.png"
    "images/Slider_bar.png"
    "images/Slider_handle.png"
    "images/Triangle_Top.png"
    "images/Triangle_bottom.png"
    "images/icon_BackArrow.png"
    "images/icon_Folder.png"
    "images/icon_Menu.png"
    "images/qt-logo.png"
    "qml/qmlvideofx/Button.qml"
    "qml/qmlvideofx/Content.qml"
    "qml/qmlvideofx/ContentCamera.qml"
    "qml/qmlvideofx/ContentImage.qml"
    "qml/qmlvideofx/ContentVideo.qml"
    "qml/qmlvideofx/Curtain.qml"
    "qml/qmlvideofx/Divider.qml"
    "qml/qmlvideofx/Effect.qml"
    "qml/qmlvideofx/EffectBillboard.qml"
    "qml/qmlvideofx/EffectBlackAndWhite.qml"
    "qml/qmlvideofx/EffectEmboss.qml"
    "qml/qmlvideofx/EffectGaussianBlur.qml"
    "qml/qmlvideofx/EffectGlow.qml"
    "qml/qmlvideofx/EffectIsolate.qml"
    "qml/qmlvideofx/EffectMagnify.qml"
    "qml/qmlvideofx/EffectPageCurl.qml"
    "qml/qmlvideofx/EffectPassThrough.qml"
    "qml/qmlvideofx/EffectPixelate.qml"
    "qml/qmlvideofx/EffectPosterize.qml"
    "qml/qmlvideofx/EffectRipple.qml"
    "qml/qmlvideofx/EffectSelectionList.qml"
    "qml/qmlvideofx/EffectSepia.qml"
    "qml/qmlvideofx/EffectSharpen.qml"
    "qml/qmlvideofx/EffectShockwave.qml"
    "qml/qmlvideofx/EffectSobelEdgeDetection1.qml"
    "qml/qmlvideofx/EffectTiltShift.qml"
    "qml/qmlvideofx/EffectToon.qml"
    "qml/qmlvideofx/EffectVignette.qml"
    "qml/qmlvideofx/EffectWarhol.qml"
    "qml/qmlvideofx/EffectWobble.qml"
    "qml/qmlvideofx/FileBrowser.qml"
    "qml/qmlvideofx/FileOpen.qml"
    "qml/qmlvideofx/HintedMouseArea.qml"
    "qml/qmlvideofx/Main.qml"
    "qml/qmlvideofx/ParameterPanel.qml"
    "qml/qmlvideofx/Slider.qml"
    "shaders/billboard.fsh"
    "shaders/blackandwhite.fsh"
    "shaders/emboss.fsh"
    "shaders/gaussianblur_h.fsh"
    "shaders/gaussianblur_v.fsh"
    "shaders/glow.fsh"
    "shaders/isolate.fsh"
    "shaders/magnify.fsh"
    "shaders/pagecurl.fsh"
    "shaders/pixelate.fsh"
    "shaders/posterize.fsh"
    "shaders/ripple.fsh"
    "shaders/selectionpanel.fsh"
    "shaders/sepia.fsh"
    "shaders/sharpen.fsh"
    "shaders/shockwave.fsh"
    "shaders/sobeledgedetection1.fsh"
    "shaders/tiltshift.fsh"
    "shaders/toon.fsh"
    "shaders/vignette.fsh"
    "shaders/warhol.fsh"
    "shaders/wobble.fsh"
)

qt6_add_resources(qmlvideofx "qmlvideofx"
    PREFIX
        "/"
    FILES
        ${qmlvideofx_resource_files}
)
set_source_files_properties("../snippets/frequencymonitor/qml/frequencymonitor/FrequencyItem.qml"
    PROPERTIES QT_RESOURCE_ALIAS "qml/frequencymonitor/FrequencyItem.qml"
)
set(frequencymonitordeclarative_resource_files
    "qml/frequencymonitor/FrequencyItem.qml"
)

qt6_add_resources(qmlvideofx "frequencymonitordeclarative"
    PREFIX
        "/"
    BASE
        "../snippets/frequencymonitor"
    FILES
        ${frequencymonitordeclarative_resource_files}
)
set_source_files_properties("../snippets/performancemonitor/qml/performancemonitor/PerformanceItem.qml"
    PROPERTIES QT_RESOURCE_ALIAS "qml/performancemonitor/PerformanceItem.qml"
)
set(performancemonitordeclarative_resource_files
    "qml/performancemonitor/PerformanceItem.qml"
)

qt6_add_resources(qmlvideofx "performancemonitordeclarative"
    PREFIX
        "/"
    BASE
        "../snippets/performancemonitor"
    FILES
        ${performancemonitordeclarative_resource_files}
)

if(ANDROID AND TARGET Qt::AndroidExtras)
    target_compile_definitions(qmlvideofx PUBLIC
        REQUEST_PERMISSIONS_ON_ANDROID
    )

    target_link_libraries(qmlvideofx PUBLIC
        Qt::AndroidExtras
    )
endif()

install(TARGETS qmlvideofx
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
