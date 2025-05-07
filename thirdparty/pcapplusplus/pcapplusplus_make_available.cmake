if("${CMAKE_SIZEOF_VOID_P}" EQUAL 8)
    if (NOT DEFINED PCAPPLUSPLUS_RELEASE_ARCHIVE_URL)
        set(PCAPPLUSPLUS_RELEASE_ARCHIVE_URL "https://github.com/seladb/PcapPlusPlus/releases/download/v24.09/pcapplusplus-24.09-windows-vs2019-x64-release.zip")
        set(PCAPPLUSPLUS_RELEASE_ARCHIVE_HASH "MD5=6070d256db039633ee5f176504d4f742")
    endif()
    if (NOT DEFINED PCAPPLUSPLUS_DEBUG_ARCHIVE_URL)
        set(PCAPPLUSPLUS_DEBUG_ARCHIVE_URL "https://github.com/seladb/PcapPlusPlus/releases/download/v24.09/pcapplusplus-24.09-windows-vs2019-x64-debug.zip")
        set(PCAPPLUSPLUS_DEBUG_ARCHIVE_HASH "MD5=1dc4d57d27b899de6f814e30e6c6807a")
    endif()
else()
    if (NOT DEFINED PCAPPLUSPLUS_RELEASE_ARCHIVE_URL)
        set(PCAPPLUSPLUS_RELEASE_ARCHIVE_URL "https://github.com/seladb/PcapPlusPlus/releases/download/v24.09/pcapplusplus-24.09-windows-vs2019-win32-release.zip")
        set(PCAPPLUSPLUS_RELEASE_ARCHIVE_HASH "MD5=0dc1cf2ea8bad3d4785c9cc6c134ce57")
    endif()
    if (NOT DEFINED PCAPPLUSPLUS_DEBUG_ARCHIVE_URL)
        set(PCAPPLUSPLUS_DEBUG_ARCHIVE_URL "https://github.com/seladb/PcapPlusPlus/releases/download/v24.09/pcapplusplus-24.09-windows-vs2019-win32-debug.zip")
        set(PCAPPLUSPLUS_DEBUG_ARCHIVE_HASH "MD5=b3c6e4a39aa49c1858a978354fa602a6")
    endif()
endif()

include(FetchContent)
FetchContent_Declare(pcapplusplus_release
    URL      "${PCAPPLUSPLUS_RELEASE_ARCHIVE_URL}"
    URL_HASH "${PCAPPLUSPLUS_RELEASE_ARCHIVE_HASH}"
    )
FetchContent_Declare(pcapplusplus_debug
    URL      "${PCAPPLUSPLUS_DEBUG_ARCHIVE_URL}"
    URL_HASH "${PCAPPLUSPLUS_DEBUG_ARCHIVE_HASH}"
    )

message(STATUS "Fetching Pcap++ (Release)...")
FetchContent_MakeAvailable(pcapplusplus_release)

message(STATUS "Fetching Pcap++ (Debug)...")
FetchContent_MakeAvailable(pcapplusplus_debug)

set(pcapplusplus_release_ROOT_DIR "${pcapplusplus_release_SOURCE_DIR}")
set(pcapplusplus_debug_ROOT_DIR "${pcapplusplus_debug_SOURCE_DIR}")

#list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/Modules/)

list(APPEND CMAKE_PREFIX_PATH ${pcapplusplus_debug_ROOT_DIR} ${pcapplusplus_release_ROOT_DIR} )
