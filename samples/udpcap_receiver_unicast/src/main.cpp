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

  // 2) Bind the socket
  //
  //    Before receiving data, the socket must be bound to an address and port.
  //    The address given here is the local address to bind to.
  // 
  //    When passing Udpcap::HostAddress::Any(), any data going to that port
  //    will be received.
  // 
  //    When passing a specific unicast address, only data that is directed to
  //    that specific IP address is received.
  //
  if (!socket.bind(Udpcap::HostAddress::Any(), 14000))
  {
    std::cerr << "ERROR: Failed to bind socket" << std::endl;
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
