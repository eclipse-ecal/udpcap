if (NOT DEFINED NPCAP_SDK_ARCHIVE_URL)
    set(NPCAP_SDK_ARCHIVE_URL  "https://npcap.com/dist/npcap-sdk-1.15.zip")
    set(NPCAP_SDK_ARCHIVE_HASH "MD5=9b89766c22d20b74bf56777821f89fe3")
endif()

include(FetchContent)
FetchContent_Declare(npcap_sdk
    URL      "${NPCAP_SDK_ARCHIVE_URL}"
    URL_HASH "${NPCAP_SDK_ARCHIVE_HASH}"
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )

message(STATUS "Fetching npcap_sdk...")
FetchContent_MakeAvailable(npcap_sdk)

# Findnpcap (for finding the npcap SDK from within UDPCAP)
set(npcap_ROOT_DIR "${npcap_sdk_SOURCE_DIR}")
set(PCAP_ROOT      "${npcap_sdk_SOURCE_DIR}") # Also set the set PCAP_ROOT for the PcapPlusPlus FindPCAP.cmake module
set(Packet_ROOT    "${npcap_sdk_SOURCE_DIR}") # Also set the set Packet_ROOT for the PcapPlusPlus FindPacket.cmake module
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/Modules/)

