include(FetchContent)
FetchContent_Declare(GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG origin/v1.16.x # This is not a Tag, but the release branch
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )

message(STATUS "Fetching GTest...")
FetchContent_MakeAvailable(GTest)

set(GTest_ROOT_DIR "${gtest_SOURCE_DIR}")

# Googletest automatically forces MT instead of MD if we do not set this option.
if(MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "My option" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "My option" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "My option" FORCE)
endif()

# Prepend googletest-module/FindGTest.cmake to Module Path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules/")