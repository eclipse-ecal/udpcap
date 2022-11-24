if (NOT DEFINED PCAPPLUSPLUS_ARCHIVE_URL)
    set(PCAPPLUSPLUS_ARCHIVE_URL "https://github.com/seladb/PcapPlusPlus/releases/download/v22.05/pcapplusplus-22.05-windows-vs2015.zip")
    set(PCAPPLUSPLUS_ARCHIVE_HASH "MD5=f0c3c289d1350fa5a5755478fef55964")
endif()

include(FetchContent)
FetchContent_Declare(pcapplusplus
    URL      "${PCAPPLUSPLUS_ARCHIVE_URL}"
    URL_HASH "${PCAPPLUSPLUS_ARCHIVE_HASH}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    )
FetchContent_GetProperties(pcapplusplus)
if(NOT pcapplusplus_POPULATED)
    message(STATUS "Fetching Pcap++...")
    FetchContent_Populate(pcapplusplus)
endif()
set(pcapplusplus_ROOT_DIR "${pcapplusplus_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/Modules/)