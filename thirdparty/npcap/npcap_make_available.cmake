if (NOT DEFINED NPCAP_SDK_ARCHIVE_URL)
    set(NPCAP_SDK_ARCHIVE_URL  "https://npcap.com/dist/npcap-sdk-1.13.zip")
    set(NPCAP_SDK_ARCHIVE_HASH "MD5=2067b3975763ddf61d4114d28d9d6c9b")
endif()

include(FetchContent)
FetchContent_Declare(npcap_sdk
    URL      "${NPCAP_SDK_ARCHIVE_URL}"
    URL_HASH "${NPCAP_SDK_ARCHIVE_HASH}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    )
FetchContent_GetProperties(npcap_sdk)
if(NOT npcap-sdk_POPULATED)
    message(STATUS "Fetching npcap_sdk...")
    FetchContent_Populate(npcap_sdk)
endif()
set(npcap_ROOT_DIR "${npcap_sdk_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/Modules/)