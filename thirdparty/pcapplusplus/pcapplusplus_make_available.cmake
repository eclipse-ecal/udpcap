include(FetchContent)
FetchContent_Declare(pcapplusplus
    GIT_REPOSITORY https://github.com/seladb/PcapPlusPlus.git
    GIT_TAG v25.05
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )

# 2025-05-15: The PCAPPP_INSTALL must be a cache variable, as
# Pacp++ v25.05 uses CMake 3.12 policies. These old CMake versions
#prevented setting options from normal variables.
#https://cmake.org/cmake/help/latest/policy/CMP0077.html
set(PCAPPP_INSTALL ON CACHE BOOL "" FORCE) 

message(STATUS "Fetching Pcap++...")
FetchContent_MakeAvailable(pcapplusplus)

# Add alias targets for PcapPlusPlus

if(NOT TARGET PcapPlusPlus::Common++)
    add_library(PcapPlusPlus::Common++ ALIAS Common++)
endif()

if(NOT TARGET PcapPlusPlus::Pcap++)
    add_library(PcapPlusPlus::Pcap++ ALIAS Pcap++)
endif()

if(NOT TARGET PcapPlusPlus::Packet++)
    add_library(PcapPlusPlus::Packet++ ALIAS Packet++)
endif()

# Disable warnings as errors for Pcap++. They have a lot of warnings but treat
# them as errors, which breaks our build, if the warnings are enabled.
set_property(TARGET Common++ PROPERTY COMPILE_WARNING_AS_ERROR OFF)
set_property(TARGET Pcap++   PROPERTY COMPILE_WARNING_AS_ERROR OFF)
set_property(TARGET Packet++ PROPERTY COMPILE_WARNING_AS_ERROR OFF)

# Also, disable the compiler warnings for those targets. It's not our code, so
# we don't want to see their warnings.
target_compile_options(Common++ PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
            -w>
        $<$<CXX_COMPILER_ID:MSVC>:
            /W0>)

target_compile_options(Pcap++ PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
            -w>
        $<$<CXX_COMPILER_ID:MSVC>:
            /W0>)

target_compile_options(Packet++ PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
            -w>
        $<$<CXX_COMPILER_ID:MSVC>:
            /W0>)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules/")
