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

project(asio_sender_unicast)

find_package(Threads REQUIRED)
find_package(asio REQUIRED)

set(sources
    src/main.cpp
)

add_executable (${PROJECT_NAME}
    ${sources}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Threads::Threads
        $<$<BOOL:${WIN32}>:ws2_32>
        $<$<BOOL:${WIN32}>:wsock32>

        # Link header-only libs (asio) as described in this workaround:
        # https://gitlab.kitware.com/cmake/cmake/-/issues/15415#note_633938
        $<BUILD_INTERFACE:asio::asio>
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        ASIO_STANDALONE
        ASIO_DISABLE_VISIBILITY
        _WIN32_WINNT=0x0601
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)
