# - Try to find PcapPlusPlus include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(PcapPlusPlus)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  PcapPlusPlus_ROOT_DIR     Set this variable to the root directory of
#                            the PcapPlusPlus SDK
#
# Variables defined by this module:
#
#  PcapPlusPlus_FOUND        System has PcapPlusPlus, include and library dirs found
#  PcapPlusPlus_INCLUDE_DIR  The PcapPlusPlus include directories.
#  PcapPlusPlus_LIBS         The PcapPlusPlus libraries

# Root dir
find_path(PcapPlusPlus_ROOT_DIR
    NAMES
        header/PcapFileDevice.h
        header/Packet.h
)

# Include dir
find_path(PcapPlusPlus_INCLUDE_DIR
    NAMES
        PcapFileDevice.h
        Packet.h
    HINTS
        "${PcapPlusPlus_ROOT_DIR}/header"
)

# Lib dir
if(("${CMAKE_GENERATOR_PLATFORM}" MATCHES "x64") OR ("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)"))
    # Find 64-bit libraries
    find_path (PcapPlusPlus_LIB_DIR
        NAMES "Release/Pcap++.lib"
        HINTS "${PcapPlusPlus_ROOT_DIR}/x64/"
    )
else()
    # Find 32-bit libraries
    find_path (PcapPlusPlus_LIB_DIR
        NAMES "Release/Pcap++.lib"
        HINTS "${PcapPlusPlus_ROOT_DIR}/x86/"
    )
endif()

# Lib files
set(PcapPlusPlus_LIBS
    optimized "${PcapPlusPlus_LIB_DIR}/Release/Common++.lib" 
    optimized "${PcapPlusPlus_LIB_DIR}/Release/Packet++.lib" 
    optimized "${PcapPlusPlus_LIB_DIR}/Release/Pcap++.lib" 

    debug "${PcapPlusPlus_LIB_DIR}/Debug/Common++.lib" 
    debug "${PcapPlusPlus_LIB_DIR}/Debug/Packet++.lib" 
    debug "${PcapPlusPlus_LIB_DIR}/Debug/Pcap++.lib" 
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PcapPlusPlus DEFAULT_MSG
    PcapPlusPlus_LIBS
    PcapPlusPlus_INCLUDE_DIR
)

# if (PcapPlusPlus_FOUND AND NOT TARGET PcapPlusPlus::PcapPlusPlus)
  # add_library(PcapPlusPlus::PcapPlusPlus UNKNOWN IMPORTED)
  # set_target_properties(PcapPlusPlus::PcapPlusPlus PROPERTIES
                        # INTERFACE_INCLUDE_DIRECTORIES "${PcapPlusPlus_INCLUDE_DIR}"
                        # IMPORTED_LOCATION ${PcapPlusPlus_LIBS})
# endif ()

if (PcapPlusPlus_FOUND AND NOT TARGET PcapPlusPlus::PcapPlusPlus)

    add_library(PcapPlusPlus::Common UNKNOWN IMPORTED)
    set_target_properties(PcapPlusPlus::Common PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${PcapPlusPlus_INCLUDE_DIR}"
                        IMPORTED_LOCATION       "${PcapPlusPlus_LIB_DIR}/Release/Common++.lib"
                        IMPORTED_LOCATION_DEBUG "${PcapPlusPlus_LIB_DIR}/Debug/Common++.lib")
                        
    add_library(PcapPlusPlus::Packet UNKNOWN IMPORTED)
    set_target_properties(PcapPlusPlus::Packet PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${PcapPlusPlus_INCLUDE_DIR}"
                        IMPORTED_LOCATION       "${PcapPlusPlus_LIB_DIR}/Release/Packet++.lib"
                        IMPORTED_LOCATION_DEBUG "${PcapPlusPlus_LIB_DIR}/Debug/Packet++.lib")

    add_library(PcapPlusPlus::Pcap UNKNOWN IMPORTED)
    set_target_properties(PcapPlusPlus::Pcap PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${PcapPlusPlus_INCLUDE_DIR}"
                        IMPORTED_LOCATION       "${PcapPlusPlus_LIB_DIR}/Release/Pcap++.lib"
                        IMPORTED_LOCATION_DEBUG "${PcapPlusPlus_LIB_DIR}/Debug/Pcap++.lib")
                        
    add_library(PcapPlusPlus::PcapPlusPlus INTERFACE IMPORTED)
    set_property(TARGET PcapPlusPlus::PcapPlusPlus PROPERTY
                        INTERFACE_LINK_LIBRARIES
                            PcapPlusPlus::Common
                            PcapPlusPlus::Packet
                            PcapPlusPlus::Pcap)

endif()

mark_as_advanced(
    PcapPlusPlus_ROOT_DIR
    PcapPlusPlus_INCLUDE_DIR
    PcapPlusPlus_LIB_DIR
    PcapPlusPlus_LIBS
)