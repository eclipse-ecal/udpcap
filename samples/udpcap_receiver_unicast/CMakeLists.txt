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

project(udpcap_receiver_unicast)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG  TRUE)
find_package(udpcap REQUIRED)

set(sources
    src/main.cpp
)

add_executable (${PROJECT_NAME}
    ${sources}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)

target_link_libraries (${PROJECT_NAME}
    udpcap::udpcap
)
