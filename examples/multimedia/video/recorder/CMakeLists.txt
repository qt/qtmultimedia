cmake_minimum_required(VERSION 3.16)

project(recorder LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/recorder")

find_package(QT NAMES Qt6 COMPONENTS Core Quick Multimedia REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Multimedia Quick)

set(PROJECT_SOURCES
        main.cpp
)

qt_add_executable(recorder
    ${PROJECT_SOURCES}
)

set(resource_files
    "main.qml"
    "qmldir"
    "MediaList.qml"
    "AudioInputSelect.qml"
    "CameraSelect.qml"
    "RecordButton.qml"
    "Controls.qml"
    "StyleParameter.qml"
    "StyleRectangle.qml"
    "SettingsMetaData.qml"
    "SettingsEncoder.qml"
    "StyleSlider.qml"
    "Style.qml"
    "Playback.qml"
)

qt6_add_resources(recorder "recorder"
    PREFIX
        "/"
    FILES
        ${resource_files}
)

target_compile_definitions(recorder
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(recorder
  PRIVATE Qt6::Core Qt6::Quick Qt6::Multimedia)

set_target_properties(recorder PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
        "${CMAKE_CURRENT_LIST_DIR}/recorder.entitlements"
)

if(APPLE AND NOT IOS)
    if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
        add_custom_command(TARGET recorder
        POST_BUILD
        COMMAND codesign --sign - --entitlements ${CMAKE_CURRENT_SOURCE_DIR}/recorder.entitlements ${CMAKE_CURRENT_BINARY_DIR}/recorder.app
      )
    endif()
endif()

install(TARGETS recorder
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
