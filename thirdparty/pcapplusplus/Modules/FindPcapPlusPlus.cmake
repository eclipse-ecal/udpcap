# - Try to find pcapplusplus include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(pcapplusplus)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  pcapplusplus_ROOT_DIR     Set this variable to the root directory of
#                            the pcapplusplus SDK
#
# Targets created by this module:
#   
#  pcapplusplus::pcapplusplus    Interface target for all libraries below
#  pcapplusplus::common          Common++.lib
#  pcapplusplus::packet          Packet++.lib
#  pcapplusplus::pcap            Pcap++.lib
#
# Variables defined by this module:
#
#  pcapplusplus_FOUND        System has pcapplusplus, include and library dirs found
#  pcapplusplus_INCLUDE_DIR  The pcapplusplus include directories.
#  pcapplusplus_LIBS         The pcapplusplus libraries
#

# Root dir
find_path(pcapplusplus_ROOT_DIR
    NAMES
        header/PcapFileDevice.h
        header/Packet.h
)

# Include dir
find_path(pcapplusplus_INCLUDE_DIR
    NAMES
        PcapFileDevice.h
        Packet.h
    HINTS
        "${pcapplusplus_ROOT_DIR}/header"
)

# Lib dir
if(("${CMAKE_GENERATOR_PLATFORM}" MATCHES "x64") OR ("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)"))
    # Find 64-bit libraries
    find_path (pcapplusplus_LIB_DIR
        NAMES "Release/Pcap++.lib"
        HINTS "${pcapplusplus_ROOT_DIR}/x64/"
    )
else()
    # Find 32-bit libraries
    find_path (pcapplusplus_LIB_DIR
        NAMES "Release/Pcap++.lib"
        HINTS "${pcapplusplus_ROOT_DIR}/x86/"
    )
endif()

# Lib files
set(pcapplusplus_LIBS
    optimized "${pcapplusplus_LIB_DIR}/Release/Common++.lib" 
    optimized "${pcapplusplus_LIB_DIR}/Release/Packet++.lib" 
    optimized "${pcapplusplus_LIB_DIR}/Release/Pcap++.lib" 

    debug "${pcapplusplus_LIB_DIR}/Debug/Common++.lib" 
    debug "${pcapplusplus_LIB_DIR}/Debug/Packet++.lib" 
    debug "${pcapplusplus_LIB_DIR}/Debug/Pcap++.lib" 
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(pcapplusplus DEFAULT_MSG
    pcapplusplus_LIBS
    pcapplusplus_INCLUDE_DIR
)

if (pcapplusplus_FOUND AND NOT TARGET pcapplusplus::pcapplusplus)

    add_library(pcapplusplus::common UNKNOWN IMPORTED)
    set_target_properties(pcapplusplus::common PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${pcapplusplus_INCLUDE_DIR}"
                        IMPORTED_LOCATION       "${pcapplusplus_LIB_DIR}/Release/Common++.lib"
                        IMPORTED_LOCATION_DEBUG "${pcapplusplus_LIB_DIR}/Debug/Common++.lib")
                        
    add_library(pcapplusplus::packet UNKNOWN IMPORTED)
    set_target_properties(pcapplusplus::packet PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${pcapplusplus_INCLUDE_DIR}"
                        IMPORTED_LOCATION       "${pcapplusplus_LIB_DIR}/Release/Packet++.lib"
                        IMPORTED_LOCATION_DEBUG "${pcapplusplus_LIB_DIR}/Debug/Packet++.lib")

    add_library(pcapplusplus::pcap UNKNOWN IMPORTED)
    set_target_properties(pcapplusplus::pcap PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${pcapplusplus_INCLUDE_DIR}"
                        IMPORTED_LOCATION       "${pcapplusplus_LIB_DIR}/Release/Pcap++.lib"
                        IMPORTED_LOCATION_DEBUG "${pcapplusplus_LIB_DIR}/Debug/Pcap++.lib")
                        
    add_library(pcapplusplus::pcapplusplus INTERFACE IMPORTED)
    set_property(TARGET pcapplusplus::pcapplusplus PROPERTY
                        INTERFACE_LINK_LIBRARIES
                            pcapplusplus::common
                            pcapplusplus::packet
                            pcapplusplus::pcap)

endif()

mark_as_advanced(
    pcapplusplus_ROOT_DIR
    pcapplusplus_INCLUDE_DIR
    pcapplusplus_LIB_DIR
    pcapplusplus_LIBS
)