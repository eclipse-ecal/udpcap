/* =========================== LICENSE =================================
 *
 * Copyright (C) 2016 - 2022 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * =========================== LICENSE =================================
 */

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
  //    There are 2 receiveDatagram() functions available. One of them returns
  //    the data as std::vector, the other expects a pointer to pre-allocated
  //    memory along with the maximum size.
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

    // Blocking receive a datagram
    std::vector<char> received_datagram = socket.receiveDatagram(&sender_address, &sender_port);

    if (sender_address.isValid())
    {
      std::cout << "Received " << received_datagram.size() << " bytes from " << sender_address.toString() << ":" << sender_port << ": " << std::string(received_datagram.data(), received_datagram.size()) << std::endl;
    }
    else
    {
      std::cerr << "ERROR: Failed to receive data from Udpcap Socket" << std::endl;
    }
  }

  return 0;
}
