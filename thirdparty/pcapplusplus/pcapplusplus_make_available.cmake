include(FetchContent)
FetchContent_Declare(pcapplusplus
    GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
    GIT_TAG cb97f6e7d22cbacd6a5ad843356dc6be012fa7e1 # 2025-05-07: The latest release 24.09 is not CMake 4.0 ready, so I am using the latest master, here
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )

message(STATUS "Fetching Pcap++...")
FetchContent_MakeAvailable(pcapplusplus)

if(NOT TARGET PcapPlusPlus::Pcap++)
    add_library(PcapPlusPlus::Pcap++ ALIAS Pcap++)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules/")
