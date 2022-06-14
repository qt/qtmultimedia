qt_examples_build_begin(EXTERNAL_BUILD)

add_subdirectory(multimedia)
if (QT_FEATURE_spatialaudio)
    add_subdirectory(spatialaudio)
endif()
add_subdirectory(multimediawidgets)

qt_examples_build_end()
