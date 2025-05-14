include(FetchContent)
FetchContent_Declare(pcapplusplus
    GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
    GIT_TAG v25.05
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )

set(PCAPPP_INSTALL ON)

message(STATUS "Fetching Pcap++...")
FetchContent_MakeAvailable(pcapplusplus)

if(NOT TARGET PcapPlusPlus::Pcap++)
    add_library(PcapPlusPlus::Pcap++ ALIAS Pcap++)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules/")
