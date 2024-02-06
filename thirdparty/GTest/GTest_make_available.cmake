include(FetchContent)
FetchContent_Declare(GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG origin/v1.14.x # This is not a Tag, but the release branch
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    )
FetchContent_GetProperties(GTest)
if(NOT gtest_POPULATED)
    message(STATUS "Fetching GTest...")
    FetchContent_Populate(GTest)
endif()
set(GTest_ROOT_DIR "${gtest_SOURCE_DIR}")

# Googletest automatically forces MT instead of MD if we do not set this option.
if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "My option" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "My option" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "My option" FORCE)
endif()

add_subdirectory("${GTest_ROOT_DIR}" EXCLUDE_FROM_ALL)

if(NOT TARGET GTest::gtest)
    add_library(GTest::gtest ALIAS gtest)
endif()

if(NOT TARGET GTest::gtest_main)
    add_library(GTest::gtest_main ALIAS gtest_main)
endif()

# Prepend googletest-module/FindGTest.cmake to Module Path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules/")