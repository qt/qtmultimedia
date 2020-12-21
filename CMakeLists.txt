# Generated from qtmultimedia.pro.

cmake_minimum_required(VERSION 3.15.0)

include(.cmake.conf)
project(QtMultimedia
    VERSION "${QT_REPO_MODULE_VERSION}"
    DESCRIPTION "Qt Multimedia Libraries"
    HOMEPAGE_URL "https://qt.io/"
    LANGUAGES CXX C
)

# special case begin
find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS BuildInternals Core Gui Network)
# find_package(Qt6 ${PROJECT_VERSION} CONFIG OPTIONAL_COMPONENTS SET_ME_TO_SOMETHING_USEFUL)
# special case end

if(NOT TARGET Qt::Gui)
    message(NOTICE "Skipping the build as the condition \"TARGET Qt::Gui\" is not met.")
    return()
endif()
if(NOT TARGET Qt::Network)
    message(NOTICE "Skipping the build as the condition \"TARGET Qt::Network\" is not met.")
    return()
endif()
qt_build_repo()
