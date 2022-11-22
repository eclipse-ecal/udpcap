# - Try to find Npcap include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(Npcap)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  Npcap_ROOT_DIR            Set this variable to the root directory of
#                            the Npcap SDK
#
# Variables defined by this module:
#
#  Npcap_FOUND                System has Npcap, include and library dirs found
#  Npcap_INCLUDE_DIR          The Npcap include directories.
#  Npcap_LIBS                 The Npcap libraries

# Root dir
find_path(Npcap_ROOT_DIR
    NAMES
        Include/pcap.h
)

# Include dir
find_path(Npcap_INCLUDE_DIR
    NAMES
        pcap.h
    HINTS
        "${Npcap_ROOT_DIR}/Include"
)

# Lib dir
if(("${CMAKE_GENERATOR_PLATFORM}" MATCHES "x64") OR ("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)"))
    # Find 64-bit libraries
    find_path (Npcap_LIB_DIR
        NAMES wpcap.lib
        HINTS "${Npcap_ROOT_DIR}/Lib/x64/"
    )
else()
    # Find 32-bit libraries
    find_path (Npcap_LIB_DIR
        NAMES wpcap.lib
        HINTS "${Npcap_ROOT_DIR}/Lib/"
    )
endif()

# Lib files
set(Npcap_LIBS
  "${Npcap_LIB_DIR}/Packet.lib" 
  "${Npcap_LIB_DIR}/wpcap.lib" 
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Npcap DEFAULT_MSG
    Npcap_LIBS
    Npcap_INCLUDE_DIR
)

# if (Npcap_FOUND AND NOT TARGET Npcap::Npcap)
  # add_library(Npcap::Npcap UNKNOWN IMPORTED)
  # set_target_properties(Npcap::Npcap PROPERTIES
                        # INTERFACE_INCLUDE_DIRECTORIES "${Npcap_INCLUDE_DIR}"
                        # IMPORTED_LOCATION ${Npcap_LIBS})
# endif ()

if (Npcap_FOUND AND NOT TARGET Npcap::Npcap)

    add_library(Npcap::wpcap UNKNOWN IMPORTED)
    set_target_properties(Npcap::wpcap PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${Npcap_INCLUDE_DIR}"
                        IMPORTED_LOCATION "${Npcap_LIB_DIR}/wpcap.lib")
                        
    add_library(Npcap::Packet UNKNOWN IMPORTED)
    set_target_properties(Npcap::Packet PROPERTIES
                        INTERFACE_INCLUDE_DIRECTORIES "${Npcap_INCLUDE_DIR}"
                        IMPORTED_LOCATION "${Npcap_LIB_DIR}/Packet.lib")
                        
    add_library(Npcap::Npcap INTERFACE IMPORTED)
    set_property(TARGET Npcap::Npcap PROPERTY
                        INTERFACE_LINK_LIBRARIES Npcap::wpcap Npcap::Packet)

endif()

mark_as_advanced(
    Npcap_ROOT_DIR
    Npcap_INCLUDE_DIR
    Npcap_LIB_DIR
    Npcap_LIBS
)