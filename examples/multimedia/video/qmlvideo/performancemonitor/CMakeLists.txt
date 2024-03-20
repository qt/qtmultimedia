# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    message(FATAL_ERROR "This module is part of the 'qmlvideo' example, and should not be built independently.")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/multimedia/video/qmlvideo/performancemonitor")

qt_add_qml_module(performancemonitor
    URI performancemonitor
    QML_FILES
        "PerformanceItem.qml"
)

target_link_libraries(performancemonitor PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
)

install(TARGETS performancemonitor
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/qmldir
    DESTINATION "${INSTALL_EXAMPLEDIR}"
)

