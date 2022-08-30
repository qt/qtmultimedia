qt_examples_build_begin(EXTERNAL_BUILD)

add_subdirectory(multimedia)
if (QT_FEATURE_spatialaudio)
    add_subdirectory(spatialaudio)
endif()

qt_examples_build_end()
