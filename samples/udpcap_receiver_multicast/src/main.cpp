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

#include <iostream>

#include <udpcap/udpcap_socket.h>

int main()
{
  // 1) Create a Udpcap Socket.
  // 
  //    Upon creation, the Udpcap Socket will initialize Npcap. If no Npcap
  //    is installed, an error is printed to console and the socket.isValid()
  //    call will return false.
  // 
  //    If the initialization has succeeded, socket.isValid() will return true.
  // 
  Udpcap::UdpcapSocket socket;

  if (!socket.isValid())
  {
    std::cerr << "ERROR: Failed to Creater UDPcap Socket" << std::endl;
    return 1;
  }

  // 2) Enable Multicast Loopback
  //
  //    This is obviously only necessary, if you want to receive multicast
  //    traffic from loopback (i.e. coming from localhost).
  // 
  //    When not desired, you need to explicitely turn it off.
  //
  socket.setMulticastLoopbackEnabled(true);

  // 3) Bind the socket
  //
  //    Before receiving data, the socket must be bound to an address and port.
  //    Because we are using multicast traffic, we are binding to any IPv4
  //    address. This will later enable us to receive any multicast traffic
  //    sent to the specified port.
  //
  if (!socket.bind(Udpcap::HostAddress::Any(), 14000))
  {
    std::cerr << "ERROR: Failed to bind socket" << std::endl;
    return 1;
  }
  
  // 4) Join a multicast group
  //
  //    For receiving multicast traffic, the system needs to join a multicast
  //    group. You can join as many multicast groups as you wish at once and
  //    also leave them later.
  //
  if (!socket.joinMulticastGroup(Udpcap::HostAddress("239.0.0.1")))
  {
    std::cerr << "ERROR: Failed to join multicast group" << std::endl;
    return 1;
  }

  // 3) Receive data from the socket
  // 
  //    The receiveDatagram() function is used to receive data from the socket.
  //    It requires the application to allocate memory for the received data.
  //    If an error occurs, the error object is set accordingly.
  // 
  //    The socket.receiveDatagram() function is blocking. In this example we
  //    can use the applications' main thread to wait for incoming data.
  //    In your own application you may want to execute the function in its own
  //    thread.
  //
  std::cout << "Start receiving data from " << socket.localAddress().toString() << ":" << socket.localPort() << "..." << std::endl;
  for (;;)
  {
    // Initialize variables for the sender's address and port
    Udpcap::HostAddress sender_address;
    uint16_t            sender_port(0);

    // Allocate memory for the received datagram (with the maximum possible udp datagram size)
    std::vector<char> received_datagram(65536);

    // Initialize error object
    Udpcap::Error error = Udpcap::Error::OK;

    // Blocking receive a datagram
    size_t received_bytes = socket.receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error);

    if (error)
    {
      std::cerr << "ERROR while receiving data:" << error.ToString() << std::endl;
      return 1;
    }

    // Shrink the received_datagram to the actual size
    received_datagram.resize(received_bytes);
    std::cout << "Received " << received_datagram.size() << " bytes from " << sender_address.toString() << ":" << sender_port << ": " << std::string(received_datagram.data(), received_datagram.size()) << std::endl;
  }

  return 0;
}
