# - Try to find npcap include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(npcap)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  npcap_ROOT_DIR            Set this variable to the root directory of
#                            the npcap SDK
#
# Targets created by this module:
#   
#  npcap::npcap               Interface target for all libraries below
#  npcap::wpcap               wpcap.lib
#  npcap::packet              Packet.lib
#
# Variables defined by this module:
#
#  npcap_FOUND                System has npcap, include and library dirs found
#  npcap_INCLUDE_DIR          The npcap include directories.
#  npcap_LIBS                 The npcap libraries
#

# Root dir
find_path(npcap_ROOT_DIR
    NAMES
        Include/pcap.h
)

# Include dir
find_path(npcap_INCLUDE_DIR
    NAMES
        pcap.h
    HINTS
        "${npcap_ROOT_DIR}/Include"
)

# Lib dir
if(("${CMAKE_GENERATOR_PLATFORM}" MATCHES "x64") OR ("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)"))
    # Find 64-bit libraries
    find_path (npcap_LIB_DIR
        NAMES wpcap.lib
        HINTS "${npcap_ROOT_DIR}/Lib/x64/"
    )
else()
    # Find 32-bit libraries
    find_path (npcap_LIB_DIR
        NAMES wpcap.lib
        HINTS "${npcap_ROOT_DIR}/Lib/"
    )
endif()

# Lib files
set(npcap_LIBS
  "${npcap_LIB_DIR}/Packet.lib" 
  "${npcap_LIB_DIR}/wpcap.lib" 
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(npcap DEFAULT_MSG
    npcap_LIBS
    npcap_INCLUDE_DIR
)

if (npcap_FOUND AND NOT TARGET npcap::npcap)

    add_library(npcap::wpcap UNKNOWN IMPORTED)
    set_target_properties(npcap::wpcap PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${npcap_INCLUDE_DIR}"
                        IMPORTED_LOCATION "${npcap_LIB_DIR}/wpcap.lib")
                        
    add_library(npcap::packet UNKNOWN IMPORTED)
    set_target_properties(npcap::packet PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${npcap_INCLUDE_DIR}"
                        IMPORTED_LOCATION "${npcap_LIB_DIR}/Packet.lib")
                        
    add_library(npcap::npcap INTERFACE IMPORTED)
    set_property(TARGET npcap::npcap PROPERTY
                        INTERFACE_LINK_LIBRARIES npcap::wpcap npcap::packet)

endif()

mark_as_advanced(
    npcap_ROOT_DIR
    npcap_INCLUDE_DIR
    npcap_LIB_DIR
    npcap_LIBS
)