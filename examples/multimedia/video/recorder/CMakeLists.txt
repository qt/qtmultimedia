cmake_minimum_required(VERSION 3.14)

project(recorder LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 COMPONENTS Core Quick Multimedia REQUIRED)
find_package(Qt6 COMPONENTS Core Quick Multimedia REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        qml.qrc
)

qt_add_executable(recorder
    ${PROJECT_SOURCES}
)

target_compile_definitions(recorder
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(recorder
  PRIVATE Qt6::Core Qt6::Quick Qt6::Multimedia)
