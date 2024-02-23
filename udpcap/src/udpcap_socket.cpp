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

#include "udpcap/udpcap_socket.h"

#include "udpcap_socket_private.h"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace Udpcap
{
  UdpcapSocket::UdpcapSocket()
    : udpcap_socket_private_(std::make_unique<Udpcap::UdpcapSocketPrivate>())
  {}

  UdpcapSocket::~UdpcapSocket() = default;

  // Move
  UdpcapSocket& UdpcapSocket::operator=(UdpcapSocket&&)  noexcept = default;
  UdpcapSocket::UdpcapSocket(UdpcapSocket&&)             noexcept = default; 

  bool              UdpcapSocket::isValid                    () const                                                { return udpcap_socket_private_->isValid(); }

  bool              UdpcapSocket::bind                       (const HostAddress& local_address, uint16_t local_port) { return udpcap_socket_private_->bind(local_address, local_port); }

  bool              UdpcapSocket::isBound                    () const                                                { return udpcap_socket_private_->isBound(); }
  HostAddress       UdpcapSocket::localAddress               () const                                                { return udpcap_socket_private_->localAddress(); }
  uint16_t          UdpcapSocket::localPort                  () const                                                { return udpcap_socket_private_->localPort(); }

  bool              UdpcapSocket::setReceiveBufferSize       (int receive_buffer_size)                               { return udpcap_socket_private_->setReceiveBufferSize(receive_buffer_size); }

  size_t            UdpcapSocket::receiveDatagram(char* data, size_t max_len, long long timeout_ms, HostAddress* source_address, uint16_t* source_port, Udpcap::Error& error) { return udpcap_socket_private_->receiveDatagram(data, max_len, timeout_ms, source_address, source_port, error); }
  size_t            UdpcapSocket::receiveDatagram(char* data, size_t max_len, long long timeout_ms, Udpcap::Error& error)                                                     { return udpcap_socket_private_->receiveDatagram(data, max_len, timeout_ms, nullptr, nullptr, error); }
  size_t            UdpcapSocket::receiveDatagram(char* data, size_t max_len, Udpcap::Error& error)                                                                           { return udpcap_socket_private_->receiveDatagram(data, max_len, -1, nullptr, nullptr, error); }
  size_t            UdpcapSocket::receiveDatagram(char* data, size_t max_len, HostAddress* source_address, uint16_t* source_port, Udpcap::Error& error)                       { return udpcap_socket_private_->receiveDatagram(data, max_len, -1, source_address, source_port, error); }

  bool              UdpcapSocket::joinMulticastGroup         (const HostAddress& group_address)                      { return udpcap_socket_private_->joinMulticastGroup(group_address); }
  bool              UdpcapSocket::leaveMulticastGroup        (const HostAddress& group_address)                      { return udpcap_socket_private_->leaveMulticastGroup(group_address); }

  void              UdpcapSocket::setMulticastLoopbackEnabled(bool enabled)                                          { udpcap_socket_private_->setMulticastLoopbackEnabled(enabled); }
  bool              UdpcapSocket::isMulticastLoopbackEnabled () const                                                { return udpcap_socket_private_->isMulticastLoopbackEnabled(); }

  void              UdpcapSocket::close                      ()                                                      { udpcap_socket_private_->close(); }
  bool              UdpcapSocket::isClosed                   () const                                                { return udpcap_socket_private_->isClosed(); }

}