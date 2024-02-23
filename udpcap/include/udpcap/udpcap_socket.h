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

// IWYU pragma: begin_exports
#include <udpcap/error.h>
#include <udpcap/host_address.h>
#include <udpcap/udpcap_export.h>
#include <udpcap/udpcap_version.h>
// IWYU pragma: end_exports

#include <vector>
#include <memory>

/*
Differences to Winsocks:
- SetReceiveBufferSize() must be called before bind() and cannot be changed after binding. On a Winsocks Socket setting the receive buffer before binding would fail.
- Sockets are always opened shared and addresses are reused. There is no way to open a non-shared socket.
- When binding to a loopback address (e.g. 127.0.0.1), setting MulticastLoopbackEnabled=true and joining a multicast group, this implementation will receive loopback multicast traffic. Winsocks would not do that (It's not clear to me why).
*/
namespace Udpcap
{
  class UdpcapSocketPrivate;


  /**
   * @brief The UdpcapSocket is a (receive-only) UDP Socket implementation using Npcap.
   *
   * Supported features are:
   *    - Binding to an IPv4 address and a port
   *    - Setting the receive buffer size
   *    - Joining and leaving multicast groups
   *    - Enabling and disabling multicast loopback (I managed to fix the cold start issue)
   *    - Receiving unicast and multicast packages (Only one memcpy from kernel to user space memory)
   *    - Fragmented IPv4 traffic
   *
   * Non supported features:
   *    - Sending data
   *    - Setting bind flags (sockets are always opened shared)
   *    - IPv6
   *
   * Differences to a normal Winsocks bases socket:
   *    - SetReceiveBufferSize() must be called before bind() and cannot be
   *      changed after binding. On a Winsocks Socket setting the receive buffer
   *      before binding would fail.
   *    - Sockets are always opened shared and addresses are reused. There is no
   *      way to open a non-shared socket.
   *    - When binding to a loopback address (e.g. 127.0.0.1), setting
   *      MulticastLoopbackEnabled=true and joining a multicast group, this
   *      implementation will receive loopback multicast traffic. Winsocks would
   *      not do that (It's not clear to me why).
   * 
   * Thread safety:
   *    - There must only be 1 thread calling receiveDatagram() at the same time
   *    - It is safe to call close() while another thread is calling receiveDatagram()
   *    - Other modifications to the socket must not be made while another thread is calling receiveDatagram()
   */
  class UdpcapSocket
  {
  public:
    /**
     * @brief Creates a new UDP Socket
     *
     * Npcap is automatically initialized. If Npcap cannot be initialized, the
     * socket will be invalid (see isValid()).
     * The Socket is not bound and mutlicast loopback is enabled.
     */
    UDPCAP_EXPORT UdpcapSocket();
    UDPCAP_EXPORT ~UdpcapSocket();

    // Copy
    UdpcapSocket(UdpcapSocket const&)             = delete;
    UdpcapSocket& operator= (UdpcapSocket const&) = delete;

    // Move
    UDPCAP_EXPORT UdpcapSocket& operator=(UdpcapSocket&&) noexcept;
    UDPCAP_EXPORT UdpcapSocket(UdpcapSocket&&) noexcept;

    /**
     * @brief Checks whether the socket is valid (i.e. npcap has been intialized successfully)
     */
    UDPCAP_EXPORT bool isValid() const;

    /**
     * @brief Binds the socket to an address and a port
     *
     * When bound successfully, the Socket is ready to receive data. If the
     * address is HostAddress::Any(), any traffic for the given port will be
     * received.
     *
     * @param local_address The address to bind to
     * @param local_port    The port to bind to
     *
     * @return Whether binding has been successfull
     */
    UDPCAP_EXPORT bool bind(const HostAddress& local_address, uint16_t local_port);

    /**
     * @brief Returns whether the socket is in bound state
     */
    UDPCAP_EXPORT bool isBound() const;

    /**
     * @brief Returns the local address used for bind(), or HostAddress::Invalid(), if the socket is not bound
     */
    UDPCAP_EXPORT HostAddress localAddress() const;

    /**
     * @brief Returns the local port used for bind(), or 0 if the socket is not bound
     */
    UDPCAP_EXPORT uint16_t localPort() const;

    /**
     * @brief Sets the receive buffer size (non-pagable memory) in bytes
     *
     * The buffer size has to be set before binding the socket.
     *
     * @param receive_buffer_size The new receive buffer size
     * @return true if successfull
     */
    UDPCAP_EXPORT bool setReceiveBufferSize(int receive_buffer_size);

    /**
     * @brief Blocks for the given time until a packet arives and copies it to the given memory
     *
     * If the socket is not bound, this method will return immediately.
     * If a source_adress or source_port is provided, these will be filled with
     * the according information from the packet. If the given time elapses
     * before a datagram was available, no data is copied and 0 is returned.
     * 
     * Possible errors:
     *   OK                     if no error occured
     *   NPCAP_NOT_INITIALIZED  if npcap has not been initialized
     *   NOT_BOUND              if the socket hasn't been bound, yet
     *   SOCKET_CLOSED          if the socket has been closed by the user
     *   TIMEOUT                if the given timeout has elapsed and no datagram was available
     *   GNERIC_ERROR           in cases of internal libpcap errors
     * 
     * Thread safety:
     *   - This method must not be called from multiple threads at the same time
     *   - While one thread is calling this method, another thread may call close()
     *   - While one thread is calling this method, no modifications must be made to the socket (except close())
     * 
     * @param data           [out]: The destination memory
     * @param max_len        [in]:  The maximum bytes available at the destination
     * @param timeout_ms     [in]:  Maximum time to wait for a datagram in ms. If -1, the method will block until a datagram is available
     * @param source_address [out]: the sender address of the datagram
     * @param source_port    [out]: the sender port of the datagram
     * @param error          [out]: The error that occured
     *
     * @return The number of bytes copied to the data pointer
     */
    UDPCAP_EXPORT size_t receiveDatagram(char*            data
                                        , size_t          max_len
                                        , long long       timeout_ms
                                        , HostAddress*    source_address
                                        , uint16_t*       source_port
                                        , Udpcap::Error&  error);

    UDPCAP_EXPORT size_t receiveDatagram(char*            data
                                        , size_t          max_len
                                        , long long       timeout_ms
                                        , Udpcap::Error&  error);

    UDPCAP_EXPORT size_t receiveDatagram(char*            data
                                        , size_t          max_len
                                        , Udpcap::Error&  error);

    UDPCAP_EXPORT size_t receiveDatagram(char*            data
                                        , size_t          max_len
                                        , HostAddress*    source_address
                                        , uint16_t*       source_port
                                        , Udpcap::Error&  error);

    /**
     * @brief Joins the given multicast group
     *
     * When successfull, the socket will then start receiving data from that
     * multicast group.
     *
     * Joining a multicast group fails, when the Socket is invalid, not bound,
     * the given address is not a multicast address or this Socket has already
     * joined the group.
     *
     * @param group_address: The multicast group to join
     *
     * @return True if successfull
     */
    UDPCAP_EXPORT bool joinMulticastGroup(const HostAddress& group_address);

    /**
     * @brief Leave the given multicast group
     *
     * Leaving a multicast group fails, when the Socket is invalid, not bound,
     * the given address is not a multicast address or this Socket has not
     * joined the group, yet.
     *
     * @param group_address: The multicast group to leave
     *
     * @return True if sucessfull
     */
    UDPCAP_EXPORT bool leaveMulticastGroup(const HostAddress& group_address);

    /**
     * @brief Sets whether local multicast traffic should be received
     *
     * If not set, the default value is true.
     *
     * @param enables whether local multicast traffic should be received
     */
    UDPCAP_EXPORT void setMulticastLoopbackEnabled(bool enabled);

    /**
     * @return Whether local multicast receiving is enabled
     */
    UDPCAP_EXPORT bool isMulticastLoopbackEnabled() const;

    /**
     * @brief Closes the socket
     * 
     * Thread safety:
     *   - It is safe to call this method while another thread is calling receiveDatagram()
     */
    UDPCAP_EXPORT void close();

    /**
     * @brief Returns whether the socket is closed
     * 
     * @return true, if the socket is closed
     */
    UDPCAP_EXPORT bool isClosed() const;

  private:
    /** This is where the actual implementation lies. But the implementation has
     * to include many nasty header files (e.g. Windows.h), which is why we only
     * forward declared the class.
     */
    std::unique_ptr<Udpcap::UdpcapSocketPrivate> udpcap_socket_private_;
  };
}
