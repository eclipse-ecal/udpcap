include(FetchContent)
FetchContent_Declare(GTest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG origin/v1.16.x # This is not a Tag, but the release branch
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )

set(INSTALL_GTEST OFF) # Enable installation of googletest. (Projects embedding googletest may want to turn this OFF.)
set(BUILD_GMOCK   OFF) # Builds the googlemock subproject

message(STATUS "Fetching GTest...")
FetchContent_MakeAvailable(GTest)


# Prepend googletest-module/FindGTest.cmake to Module Path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules/")