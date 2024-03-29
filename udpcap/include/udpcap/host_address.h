/********************************************************************************
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

#pragma once

#include <string>

// IWYU pragma: begin_exports
#include <udpcap/udpcap_export.h>
// IWYU pragma: end_exports

namespace Udpcap
{
  /**
   * @brief Representation of a Host address
   *
   * Currently, only IPv4 addresses are supported. A HostAddress can be
   * constructed from a string (like "127.0.0.1") or a corresponding 32bit
   * integer.
   * Providing a faulty IPv4 string will result in an invalid host address
   * (check with isValid()). Using the default constructor will also result in
   * an invalid host address.
   *
   * There are several predefined Host addresses:
   *   HostAddress::Invalid()
   *   HostAddress::Any()
   *   HostAddress::LocalHost()
   *   HostAddress::Broadcast()
   *
   */
  class HostAddress
  {
  ////////////////////////////////
  // Host address
  ////////////////////////////////
  public:
    /** @brief Constructs an Invalid Host address */
    UDPCAP_EXPORT HostAddress();

    /**
     * @brief Constructs a Host address from a IPv4 string.
     * If the given tring is not parsable, the HostAddress will be inavlid.
     */
    UDPCAP_EXPORT HostAddress(const std::string& address);

    /** @brief Constructs a HostAddress from a 32bit integer in host byte order. */
    UDPCAP_EXPORT HostAddress(uint32_t address);

    /** @brief Checks if the Host Address is valid.
     * Invalid HostAddresses are created when providing a wrong IPv4 string,
     * using the empty default constructor or the HostAddress::Invalid()
     * predefined address.
     */
    UDPCAP_EXPORT bool isValid() const;

    /**
     * @brief Checks if the HostAddress is a loopback address
     *
     * The IPv4 Loopback address range is from 127.0.0.0 to 127.255.255.255.
     * Invalid addresses are not considered to be loopback.
     *
     * You should always use this function to check for loopback addresses. Do
     * not only compare it to HostAddress::LocalHost() (=> 127.0.0.1), as the
     * loopback address range consists of many more addresses than the localhost
     * address.
     *
     * @return true, if the address is a loopback address
     */
    UDPCAP_EXPORT bool isLoopback() const;

    /**
     * @brief Checks if the HostAddress is a multicast address
     *
     * The IPv4 multicast address range is from 224.0.0.0 to 239.255.255.255.
     * Invalid addresses are not considered to be multicast.
     *
     * @return true, if the address is a multicast address
     */
    UDPCAP_EXPORT bool isMulticast() const;

    /**
     * @brief Creates a string representation of the HostAddress
     *
     * If invalid, an empty string is returned.
     * For HostAddress::Any() "0.0.0.0" will be returned
     *
     * @return A string representation of the HostAddress
     */
    UDPCAP_EXPORT std::string toString() const;

    /**
     * @brief Creates a 32bit Host-order integer representation of the HostAddress
     * If invalid or Any(), 0 will be returned.
     * @return The host-order integer representation of the HostAddress
     */
    UDPCAP_EXPORT uint32_t    toInt()    const;

  private:
    bool     valid_;  /**< Whether this address is valid */
    uint32_t ipv4_;   /**< The host-order integer representation of this address */

  ////////////////////////////////
  // Compare operators
  ////////////////////////////////
  public:
    UDPCAP_EXPORT bool operator==(const HostAddress& other) const;
    UDPCAP_EXPORT bool operator!=(const HostAddress& other) const;
    UDPCAP_EXPORT bool operator< (const HostAddress& other) const;
    UDPCAP_EXPORT bool operator> (const HostAddress& other) const;
    UDPCAP_EXPORT bool operator<=(const HostAddress& other) const;
    UDPCAP_EXPORT bool operator>=(const HostAddress& other) const;

  ////////////////////////////////
  // Special addresses
  ////////////////////////////////
  public:
    /**
     * @brief Constructs an invald HostAddress (same as default constructor)
     * @return An invalid HostAddress
     */
    UDPCAP_EXPORT static HostAddress Invalid();

    /**
     * @brief Constructs a HostAddress representing any address (0.0.0.0)
     * @return A HostAddress representing any address
     */
    UDPCAP_EXPORT static HostAddress Any();

    /**
     * @brief Constructs a localhost HostAddress (127.0.0.1)
     * @return A localhost HostAddress
     */
    UDPCAP_EXPORT static HostAddress LocalHost();

    /**
     * @brief Constructs a broadcast HostAddress (255.255.255.255)
     * @return A broadcast HostAddress
     */
    UDPCAP_EXPORT static HostAddress Broadcast();
  };
}

