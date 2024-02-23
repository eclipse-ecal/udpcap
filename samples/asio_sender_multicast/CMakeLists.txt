################################################################################
# Copyright (c) 2016 Continental Corporation
# 
# This program and the accompanying materials are made available under the
# terms of the Apache License, Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0.
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
# 
# SPDX-License-Identifier: Apache-2.0
################################################################################

cmake_minimum_required(VERSION 3.13)

project(asio_sender_multicast)

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
