cmake_minimum_required(VERSION 3.16)

project(multimedia_cppsnippets)

add_executable(mytarget
    multimedia-snippets/media.cpp
)

# ![0]
find_package(Qt6 REQUIRED COMPONENTS Multimedia)
target_link_libraries(mytarget PRIVATE Qt6::Multimedia)
# ![0]
