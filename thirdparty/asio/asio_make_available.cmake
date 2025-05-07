include(FetchContent)
FetchContent_Declare(asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-34-2
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )

message(STATUS "Fetching asio...")
FetchContent_MakeAvailable(asio)

set(asio_ROOT_DIR "${asio_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/Modules/)