# =========================== LICENSE =================================
#
# Copyright (C) 2016 - 2022 Continental Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# =========================== LICENSE =================================

cmake_minimum_required(VERSION 3.13)

# Project call
include("${CMAKE_CURRENT_LIST_DIR}/udpcap/version.cmake")
project(udpcap VERSION ${UDPCAP_VERSION_MAJOR}.${UDPCAP_VERSION_MINOR}.${UDPCAP_VERSION_PATCH})

# Normalize backslashes from Windows paths
file(TO_CMAKE_PATH "${CMAKE_MODULE_PATH}" CMAKE_MODULE_PATH)
file(TO_CMAKE_PATH "${CMAKE_PREFIX_PATH}" CMAKE_PREFIX_PATH)
message(STATUS "Module Path: ${CMAKE_MODULE_PATH}")
message(STATUS "Prefix Path: ${CMAKE_PREFIX_PATH}")

# CMake Options
include (CMakeDependentOption)

option(UDPCAP_BUILD_SAMPLES
       "Build project samples"
       ON)

option(UDPCAP_THIRDPARTY_ENABLED
       "Enable building against the builtin dependencies"
       ON)

cmake_dependent_option(UDPCAP_THIRDPARTY_USE_BUILTIN_NPCAP
       "Fetch and build against a  predefined version of the npcap-sdk. If disabled, the targets have to be provided externally."
       ON
       "UDPCAP_THIRDPARTY_ENABLED"
       OFF)

cmake_dependent_option(UDPCAP_THIRDPARTY_USE_BUILTIN_PCAPPLUSPLUS
       "Fetch and build against a predefined version of Pcap++. If disabled, the targets have to be provided externally."
       ON
       "UDPCAP_THIRDPARTY_ENABLED"
       OFF)

cmake_dependent_option(UDPCAP_THIRDPARTY_USE_BUILTIN_ASIO
       "Fetch and build against a  predefined version of Asio. If disabled, the targets have to be provided externally."
       ON
       "UDPCAP_THIRDPARTY_ENABLED"
       OFF)

# Module path for finding udpcap
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/udpcap/modules)

#--- Fetch Npcap SDK -------------------------------
if (UDPCAP_THIRDPARTY_USE_BUILTIN_NPCAP)
    include(thirdparty/npcap/npcap_make_available.cmake)
endif()

#--- Fetch Pcap++ -------------------------------
if (UDPCAP_THIRDPARTY_USE_BUILTIN_PCAPPLUSPLUS)
    include(thirdparty/pcapplusplus/pcapplusplus_make_available.cmake)
endif()

#--- Fetch Asio -------------------------------
if (UDPCAP_THIRDPARTY_USE_BUILTIN_ASIO)
    include(thirdparty/asio/asio_make_available.cmake)
endif()

#----------------------------------------------

# Set Debug postfix
set(CMAKE_DEBUG_POSTFIX            d)
set(CMAKE_MINSIZEREL_POSTFIX       minsize)
set(CMAKE_RELWITHDEBINFO_POSTFIX   reldbg)

# Add main udpcap library
add_subdirectory(udpcap)

# Generic samples
if (UDPCAP_BUILD_SAMPLES)
    add_subdirectory(samples/udpcap_receiver_multicast)
    add_subdirectory(samples/udpcap_receiver_unicast)

    add_subdirectory(samples/asio_sender_multicast)
    add_subdirectory(samples/asio_sender_unicast)
endif()


# Make this package available for packing with CPack
include("${CMAKE_CURRENT_LIST_DIR}/cpack_config.cmake")
