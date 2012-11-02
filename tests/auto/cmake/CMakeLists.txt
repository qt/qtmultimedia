
cmake_minimum_required(VERSION 2.8)

project(qmake_cmake_files)

enable_testing()

find_package(Qt5Core REQUIRED)

include("${_Qt5CTestMacros}")

set(Qt5_MODULE_TEST_DEPENDS Network Widgets)

test_module_includes(
  Multimedia QCamera
  MultimediaWidgets QVideoWidget
)
