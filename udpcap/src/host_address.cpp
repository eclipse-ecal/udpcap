﻿/********************************************************************************
 * Copyright (c) 2016 Continental Corporation
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 * 
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/

#include "udpcap/host_address.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Ws2tcpip.h>

#include <array>
#include <cstdint>
#include <string>

namespace Udpcap
{
  ////////////////////////////////
  // Host address
  ////////////////////////////////

  HostAddress::HostAddress()
    : valid_(false)
    , ipv4_(0)
  {}

  HostAddress::HostAddress(const std::string& address)
    : valid_(false)
    , ipv4_(0)
  {
    valid_ = (inet_pton(AF_INET, address.c_str(), (void*)(&ipv4_)) == 1);
  }

  HostAddress::HostAddress(uint32_t address)
    : valid_(true)
    , ipv4_(address)
  {}

  bool HostAddress::isValid() const
  {
    return valid_;
  }

  bool HostAddress::isLoopback() const
  {
    if (valid_)
    {
      return (ipv4_ & 0x000000FF) == 127;
    }
    else
    {
      return false;
    }
  }

  bool HostAddress::isMulticast() const
  {
    if (valid_)
    {
      const uint32_t lower_byte = (ipv4_ & 0x000000FF);
      return (lower_byte >= 224) && (lower_byte <= 239);
    }
    else
    {
      return false;
    }
  }

  std::string HostAddress::toString() const
  {
    if (valid_)
    {
      std::array<char, 16> buffer{};
      inet_ntop(AF_INET, (void*)(&ipv4_), buffer.data(), 16);
      return std::string(buffer.data());
    }
    else
    {
      return "";
    }
  }

  uint32_t HostAddress::toInt() const
  {
    return ipv4_;
  }

  ////////////////////////////////
  // Compare operators
  ////////////////////////////////

  bool HostAddress::operator==(const HostAddress& other) const
  {
    return ((valid_ == other.valid_) && (ipv4_ == other.ipv4_));
  }

  bool HostAddress::operator!=(const HostAddress& other) const
  {
    return !operator==(other);
  }

  bool HostAddress::operator<(const HostAddress& other) const
  {
    if (!valid_ || !other.valid_)
      return false;
    return ipv4_ < other.ipv4_;
  }

  bool HostAddress::operator>(const HostAddress& other) const
  {
    if (!valid_ || !other.valid_)
      return false;
    return ipv4_ > other.ipv4_;
  }

  bool HostAddress::operator<=(const HostAddress& other) const
  {
    return (ipv4_ < other.ipv4_) || (ipv4_ == other.ipv4_);
  }

  bool HostAddress::operator>=(const HostAddress& other) const
  {
    return (ipv4_ > other.ipv4_) || (ipv4_ == other.ipv4_);
  }

  ////////////////////////////////
  // Special addresses
  ////////////////////////////////

  HostAddress HostAddress::Invalid()   { return HostAddress(); }
  HostAddress HostAddress::Any()       { return HostAddress(0x00000000UL /* = 0.0.0.0 */); };
  HostAddress HostAddress::LocalHost() { return HostAddress(0x0100007FUL /* = 127.0.0.1 */); };
  HostAddress HostAddress::Broadcast() { return HostAddress(0xFFFFFFFFUL /* = 255.255.255.255*/); };
}
