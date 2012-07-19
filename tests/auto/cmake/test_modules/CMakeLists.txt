
cmake_minimum_required(VERSION 2.8)

project(test_modules)

find_package(Qt5Network REQUIRED)
find_package(Qt5Widgets REQUIRED)

find_package(Qt5Multimedia REQUIRED)
find_package(Qt5MultimediaWidgets REQUIRED)

include_directories(
  ${Qt5Multimedia_INCLUDE_DIRS}
  ${Qt5MultimediaWidgets_INCLUDE_DIRS}
)

add_definitions(
  ${Qt5Multimedia_DEFINITIONS}
  ${Qt5MultimediaWidgets_DEFINITIONS}
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Core_EXECUTABLE_COMPILE_FLAGS}")

add_executable(mainapp main.cpp)
target_link_libraries(mainapp
  ${Qt5Multimedia_LIBRARIES}
  ${Qt5MultimediaWidgets_LIBRARIES}
)
