/********************************************************************************
 * Copyright (c) 2024 Continental Corporation
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

#include <gtest/gtest.h>

#include <udpcap/udpcap_socket.h>
#include <asio.hpp>

#include <thread>

#include "atomic_signalable.h"

// Create and destroy as UdpcapSocket
TEST(udpcap, RAII)
{
  {
    // Create a udpcap socket
    const Udpcap::UdpcapSocket udpcap_socket;
    ASSERT_TRUE(udpcap_socket.isValid());
    // Delete the socket
  }
}

// Create and destroy a abound UdpcapSocket
TEST(udpcap, RAIIWithClose)
{
  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  // bind the socket
  const bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
  ASSERT_TRUE(success);

  // Close the socket
  udpcap_socket.close();
}

// Create and destroy a bound UdpcapSocket with a thread waiting for a datagram
TEST(udpcap, RAIIWithSomebodyWaiting)
{
  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  // bind the socket
  const bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
  ASSERT_TRUE(success);
      
  // Blocking receive a datagram
  std::thread receive_thread([&udpcap_socket]()
                            {
                              // Create buffer with max udp datagram size
                              std::vector<char> received_datagram;
                              received_datagram.resize(65536);

                              Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;

                              // blocking receive
                              const size_t received_bytes =  udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), 0, error);

                              // Check that we didn't receive any bytes
                              ASSERT_EQ(received_bytes, 0);

                              ASSERT_TRUE(bool(error));

                              // Check the error reason
                              ASSERT_EQ(error, Udpcap::Error(Udpcap::Error::ErrorCode::SOCKET_CLOSED));

                              // Check that the socket is closed
                              ASSERT_TRUE(udpcap_socket.isClosed());

                            });

  // Close the socket
  udpcap_socket.close();

  // Join the thread
  receive_thread.join();

  // Delete the socket
}

// Test the return value of a bind with an invalid address
TEST(udpcap, BindInvalidAddress)
{
  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  // bind the socket
  const bool success = udpcap_socket.bind(Udpcap::HostAddress("256.0.0.1"), 14000);
  ASSERT_FALSE(success);
}

// Test the return value of a bind with a valid address that however doesn't belong to any network interface
TEST(udpcap, BindInvalidAddress2)
{
  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  // bind the socket
  const bool success = udpcap_socket.bind(Udpcap::HostAddress("239.0.0.1"), 14000); // This is a multicast address that cannot be bound to
  ASSERT_FALSE(success);
}

// Receive a simple Hello World Message
TEST(udpcap, SimpleReceive)
{
  atomic_signalable<int> received_messages(0);

  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  {
    const bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
    ASSERT_TRUE(success);
  }

  // Create an asio UDP sender socket
  asio::io_service io_service;
  const asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 14000);
  asio::ip::udp::socket         asio_socket(io_service, endpoint.protocol());
  asio_socket.connect(endpoint);
  const auto asio_local_endpoint = asio_socket.local_endpoint();

  // Blocking receive a datagram
  std::thread receive_thread([&udpcap_socket, &received_messages, &asio_local_endpoint]()
                              {
                                // Initialize variables for the sender's address and port
                                Udpcap::HostAddress sender_address;
                                uint16_t            sender_port(0);
                                Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;

                                // Allocate buffer with max udp datagram size
                                std::vector<char> received_datagram;
                                received_datagram.resize(65536);

                                // blocking receive
                                const size_t received_bytes = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error);
                                received_datagram.resize(received_bytes);

                                // No error must have occurred
                                ASSERT_FALSE(bool(error));

                                // Check if the received datagram is valid and contains "Hello World"
                                ASSERT_FALSE(received_datagram.empty());
                                ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), "Hello World");

                                // Check if the sender's address and port are correct
                                ASSERT_EQ(sender_address.toString(), asio_local_endpoint.address().to_string());
                                ASSERT_EQ(sender_port,               asio_local_endpoint.port());

                                received_messages++;
                              });

  std::string buffer_string = "Hello World";
  asio_socket.send_to(asio::buffer(buffer_string), endpoint);

  // Wait max 100ms for the receive thread to finish
  received_messages.wait_for([](int value) { return value >= 1; }, std::chrono::milliseconds(100));

  // Check if the received message counter is 1
  ASSERT_EQ(received_messages.get(), 1);

  asio_socket.close();
  udpcap_socket.close();

  receive_thread.join();
}

// Receive multiple small packages with a small delay between sending
TEST(udpcap, MultipleSmallPackages)
{
  constexpr int num_packages_to_send = 10;
  constexpr std::chrono::milliseconds send_delay(1);

  atomic_signalable<int> received_messages(0);

  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  // Bind the udpcap socket to all interfaces
  {
    const bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
    ASSERT_TRUE(success);
  }

  // Create an asio UDP sender socket
  asio::io_service io_service;
  const asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 14000);
  asio::ip::udp::socket         asio_socket(io_service, endpoint.protocol());
  asio_socket.connect(endpoint);
  const auto asio_local_endpoint = asio_socket.local_endpoint();

  // Receive datagrams in a separate thread
  std::thread receive_thread([&udpcap_socket, &received_messages, num_packages_to_send, &asio_local_endpoint]()
                              {
                                while (true)
                                {
                                  // Initialize variables for the sender's address and port
                                  Udpcap::HostAddress sender_address;
                                  uint16_t            sender_port(0);
                                  Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;

                                  // Allocate buffer with max udp datagram size
                                  std::vector<char> received_datagram;
                                  received_datagram.resize(65536);

                                  // blocking receive
                                  const size_t received_bytes = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error);

                                  if (error)
                                  {
                                    // Indicates that somebody closed the socket
                                    ASSERT_EQ(received_messages.get(), num_packages_to_send);

                                    // Check the error reason
                                    ASSERT_EQ(error, Udpcap::Error(Udpcap::Error::ErrorCode::SOCKET_CLOSED));

                                    // Check that the socket is closed
                                    ASSERT_TRUE(udpcap_socket.isClosed());

                                    break;
                                  }

                                  received_datagram.resize(received_bytes);

                                  // Check if the received datagram is valid and contains "Hello World"
                                  ASSERT_FALSE(received_datagram.empty());
                                  ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), "Hello World");

                                  // Check if the sender's address and port are correct
                                  ASSERT_EQ(sender_address.toString(), asio_local_endpoint.address().to_string());
                                  ASSERT_EQ(sender_port,               asio_local_endpoint.port());

                                  received_messages++;
                                }
                              });

  std::string buffer_string = "Hello World";
  for (int i = 0; i < num_packages_to_send; i++)
  {
    asio_socket.send_to(asio::buffer(buffer_string), endpoint);
    std::this_thread::sleep_for(send_delay);
  }

  // Wait max 100ms for the receive thread to finish
  received_messages.wait_for([num_packages_to_send](int value) { return value >= num_packages_to_send; }, std::chrono::milliseconds(100));

  // Check if the received message counter is 1
  ASSERT_EQ(received_messages.get(), num_packages_to_send);

  asio_socket.close();
  udpcap_socket.close();

  receive_thread.join();
}

// Receive a datagram after it has been sent, so it had to be buffered
TEST(udpcap, SimpleReceiveWithBuffer)
{
  atomic_signalable<int> received_messages(0);

  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  {
    const bool success = udpcap_socket.bind(Udpcap::HostAddress::LocalHost(), 14000);
    ASSERT_TRUE(success);
  }

  // Create an asio UDP sender socket
  asio::io_service io_service;
  const asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 14000);
  asio::ip::udp::socket         asio_socket(io_service, endpoint.protocol());
  asio_socket.connect(endpoint);
  const auto asio_local_endpoint = asio_socket.local_endpoint();

  // Send "Hello World" without currently polling the socket
  std::string buffer_string = "Hello World";
  asio_socket.send_to(asio::buffer(buffer_string), endpoint);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Receive the datagram
  std::thread receive_thread([&udpcap_socket, &received_messages, &asio_local_endpoint]()
                              {
                                // Initialize variables for the sender's address and port
                                Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;
                                Udpcap::HostAddress sender_address;
                                uint16_t            sender_port(0);

                                // Create buffer with max udp datagram size
                                std::vector<char> received_datagram;
                                received_datagram.resize(65536);

                                received_datagram.resize(udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error));

                                // No error must have occurred
                                ASSERT_FALSE(bool(error));

                                // Check if the received datagram is valid and contains "Hello World"
                                ASSERT_FALSE(received_datagram.empty());
                                ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), "Hello World");

                                // Check if the sender's address and port are correct
                                ASSERT_EQ(sender_address.toString(), asio_local_endpoint.address().to_string());
                                ASSERT_EQ(sender_port,               asio_local_endpoint.port());

                                received_messages++;
                              });


  // Wait max 100ms for the receive thread to finish
  received_messages.wait_for([](int value) { return value >= 1; }, std::chrono::milliseconds(100));

  // Check if the received message counter is 1
  ASSERT_EQ(received_messages.get(), 1);

  asio_socket.close();
  udpcap_socket.close();

  receive_thread.join();
}

// Receive multiple datagrams slower than they are sent, so they have to be buffered
TEST(udpcap, DelayedPackageReceiveMultiplePackages)
{
  constexpr int num_packages_to_send = 100;
  constexpr int size_per_package     = 1024;
  constexpr std::chrono::milliseconds receive_delay(10);

  atomic_signalable<int> received_messages(0);

  // Create a 1400 byte buffer for sending
  std::vector<char> buffer(size_per_package, 'a');

  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  // Bind the udpcap socket to all interfaces
  {
    const bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
    ASSERT_TRUE(success);
  }

  // Create an asio UDP sender socket
  asio::io_service io_service;
  const asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 14000);
  asio::ip::udp::socket         asio_socket(io_service, endpoint.protocol());
  asio_socket.connect(endpoint);
  const auto asio_local_endpoint = asio_socket.local_endpoint();


  // Receive datagrams in a separate thread
  std::thread receive_thread([&udpcap_socket, &received_messages, num_packages_to_send, size_per_package, receive_delay, &asio_local_endpoint]()
                              {
                                while (true)
                                {
                                  // Initialize variables for the sender's address and port
                                  Udpcap::HostAddress sender_address;
                                  uint16_t            sender_port(0);

                                  Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;

                                  std::vector<char> received_datagram;
                                  received_datagram.resize(65536);

                                  const size_t bytes_received = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error);
                                  received_datagram.resize(bytes_received);

                                  if (error)
                                  {
                                    // Indicates that somebody closed the socket
                                    ASSERT_EQ(received_messages.get(), num_packages_to_send);

                                    // Check the error reason
                                    ASSERT_EQ(error, Udpcap::Error(Udpcap::Error::ErrorCode::SOCKET_CLOSED));

                                    // Check that the socket is closed
                                    ASSERT_TRUE(udpcap_socket.isClosed());

                                    break;
                                  }

                                  // Check if the received datagram is valid and contains "Hello World"
                                  ASSERT_EQ(received_datagram.size(), size_per_package);
                                  received_messages++;

                                  // Check the sender endpoint
                                  ASSERT_EQ(sender_address.toString(), asio_local_endpoint.address().to_string());
                                  ASSERT_EQ(sender_port,               asio_local_endpoint.port());

                                  // Wait a bit, so we force the udpcap socket to buffer the datagrams
                                  std::this_thread::sleep_for(receive_delay);
                                }
                              });

  // Send the buffers
  for (int i = 0; i < num_packages_to_send; i++)
  {
    asio_socket.send_to(asio::buffer(buffer), endpoint);
  }

  // Wait some time for the receive thread to finish
  received_messages.wait_for([num_packages_to_send](int value) { return value >= num_packages_to_send; }, receive_delay * num_packages_to_send + std::chrono::milliseconds(2000));

  // Check if the received message counter is equal to the sent messages
  ASSERT_EQ(received_messages.get(), num_packages_to_send);

  asio_socket.close();
  udpcap_socket.close();

  receive_thread.join();
}

// Test the timeout of the receiveDatagram function
TEST(udpcap, Timeout)
{
  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());
    
  {
    const bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
    ASSERT_TRUE(success);
  }
    
  // Initialize variables for the sender's address and port
  Udpcap::HostAddress sender_address;
  uint16_t            sender_port(0);
  Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;

  // Intialize an asio socket
  asio::io_service io_service;
  const asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 14000);
  asio::ip::udp::socket         asio_socket(io_service, endpoint.protocol());
  std::string buffer_string = "Hello World";

    
  // Allocate buffer with max udp datagram size
  std::vector<char> received_datagram;
  received_datagram.resize(65536);
  
  // Nothing is received while waiting
  {
    // Take Start time
    auto start_time = std::chrono::steady_clock::now();

    // blocking receive with a 100ms timeout
    const size_t received_bytes = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), 100, &sender_address, &sender_port, error);

    // Take End time
    auto end_time = std::chrono::steady_clock::now();
  
    ASSERT_EQ(error, Udpcap::Error::TIMEOUT);
    ASSERT_EQ(received_bytes, 0); 

    ASSERT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count(), 100); // TODO: This sometimes fails. Check why!
  }

  // Something already is in the socket, so the call must return earlier
  {
    asio_socket.send_to(asio::buffer(buffer_string), endpoint);

    // sleep 10ms
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Take Start time
    auto start_time = std::chrono::steady_clock::now();

    // blocking receive with a 500ms timeout
    const size_t received_bytes = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), 500, &sender_address, &sender_port, error);

    // Take End time
    auto end_time = std::chrono::steady_clock::now();
  
    ASSERT_EQ(error, Udpcap::Error::OK);
    ASSERT_EQ(received_bytes, buffer_string.size());
    ASSERT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count(), 500);

    // Resize the buffer and check the content
    received_datagram.resize(received_bytes);
    ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), buffer_string);
  }

  // 0ms timeout returns immediately, when nothing is in the socket
  {
    // Take Start time
    auto start_time = std::chrono::steady_clock::now();
    
    // blocking receive with a 0ms timeout
    const size_t received_bytes = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), 0, &sender_address, &sender_port, error);
    
    // Take End time
    auto end_time = std::chrono::steady_clock::now();
    
    ASSERT_EQ(error, Udpcap::Error::TIMEOUT);
    ASSERT_EQ(received_bytes, 0);
    ASSERT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count(), 100);
  }

  // 0ms timeout returns immediately when something is in the socket
  {
    asio_socket.send_to(asio::buffer(buffer_string), endpoint);
    
    // Take Start time
    auto start_time = std::chrono::steady_clock::now();
        
    // blocking receive with a 0ms timeout
    const size_t received_bytes = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), 0, &sender_address, &sender_port, error);
        
    // Take End time
    auto end_time = std::chrono::steady_clock::now();
        
    ASSERT_EQ(error, Udpcap::Error::OK);
    ASSERT_EQ(received_bytes, buffer_string.size());
    ASSERT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count(), 100);

    // Resize the buffer and check the content
    received_datagram.resize(received_bytes);
    ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), buffer_string);
  }

  // Close the socket
  udpcap_socket.close();
}

// Test receiving without binding the socket (-> error)
TEST(udpcap, ReceiveNotBound)
{
  // Create a udpcap socket
    Udpcap::UdpcapSocket udpcap_socket;
    ASSERT_TRUE(udpcap_socket.isValid());
    
    // Initialize variables for the sender's address and port
    Udpcap::HostAddress sender_address;
    uint16_t            sender_port(0);
    
    // Allocate buffer with max udp datagram size
    std::vector<char> received_datagram;
    received_datagram.resize(65536);
    
    // Initialize error object
    Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;
    
    // blocking receive
    const size_t received_bytes = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error);
    
    // Check if the received datagram is valid and contains "Hello World"
    ASSERT_EQ(received_bytes, 0);
    ASSERT_TRUE(bool(error));
    ASSERT_EQ(error, Udpcap::Error(Udpcap::Error::ErrorCode::NOT_BOUND));
}

// Test the multicast functionality
TEST(udpcap, MulticastReceive)
{
  atomic_signalable<int> received_messages1(0);
  atomic_signalable<int> received_messages2(0);

  // Create two udpcap sockets
  Udpcap::UdpcapSocket udpcap_socket1;
  ASSERT_TRUE(udpcap_socket1.isValid());

  Udpcap::UdpcapSocket udpcap_socket2;
  ASSERT_TRUE(udpcap_socket2.isValid());

  udpcap_socket1.setMulticastLoopbackEnabled(true);
  udpcap_socket2.setMulticastLoopbackEnabled(true);

  // Bind the udpcap sockets to all interfaces
  {
    const bool success = udpcap_socket1.bind(Udpcap::HostAddress::Any(), 14000);
    ASSERT_TRUE(success);
  }
  {
    const bool success = udpcap_socket2.bind(Udpcap::HostAddress::Any(), 14000);
    ASSERT_TRUE(success);
  }

  // Join the multicast group 224.0.0.1 on both sockets
  {
    const bool success = udpcap_socket1.joinMulticastGroup(Udpcap::HostAddress("224.0.0.1"));
    ASSERT_TRUE(success);
  }
  {
    const bool success = udpcap_socket2.joinMulticastGroup(Udpcap::HostAddress("224.0.0.1"));
    ASSERT_TRUE(success);
  }

  // Join the multicast group 224.0.0.2 on the second socket
  {
    const bool success = udpcap_socket2.joinMulticastGroup(Udpcap::HostAddress("224.0.0.2"));
    ASSERT_TRUE(success);
  }

  // Create an asio UDP sender socket
  asio::io_service      io_service;
  asio::ip::udp::socket asio_socket(io_service, asio::ip::udp::v4());

  // open the socket for multicast sending
  asio_socket.set_option(asio::ip::multicast::hops(1));
  asio_socket.set_option(asio::ip::multicast::enable_loopback(true));

  // Receive datagrams in a separate thread for Socket1 (checks for 224.0.0.1)
  std::thread receive_thread1([&udpcap_socket1, &received_messages1]()
                              {
                                while (true)
                                {
                                  // Initialize variables for the sender's address and port
                                  Udpcap::HostAddress sender_address;
                                  uint16_t            sender_port(0);
    
                                  Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;
    
                                  std::vector<char> received_datagram;
                                  received_datagram.resize(65536);
    
                                  const size_t bytes_received = udpcap_socket1.receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error);
                                  received_datagram.resize(bytes_received);
    
                                  if (error)
                                  {
                                    // Indicates that somebody closed the socket
                                    ASSERT_EQ(received_messages1.get(), 1);

                                    // Check the error reason
                                    ASSERT_EQ(error, Udpcap::Error(Udpcap::Error::ErrorCode::SOCKET_CLOSED));

                                    // Check that the socket is closed
                                    ASSERT_TRUE(udpcap_socket1.isClosed());

                                    break;
                                  }
    
                                  // Check if the received datagram is valid and contains "224.0.0.1"
                                  ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), "224.0.0.1");
                                  received_messages1++;
                                }
                              });

  // Receive datagrams in a separate thread for Socket2 (checks for 224.0.0.1 or 224.0.0.2) 
  std::thread receive_thread2([&udpcap_socket2, &received_messages2]()
                                {
                                  while (true)
                                  {
                                    // Initialize variables for the sender's address and port
                                    Udpcap::HostAddress sender_address;
                                    uint16_t            sender_port(0);
        
                                    Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;
        
                                    std::vector<char> received_datagram;
                                    received_datagram.resize(65536);
        
                                    const size_t bytes_received = udpcap_socket2.receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error);
                                    received_datagram.resize(bytes_received);
        
                                    if (error)
                                    {
                                      // Indicates that somebody closed the socket
                                      ASSERT_EQ(received_messages2.get(), 2);

                                      // Check the error reason
                                      ASSERT_EQ(error, Udpcap::Error(Udpcap::Error::ErrorCode::SOCKET_CLOSED));

                                      // Check that the socket is closed
                                      ASSERT_TRUE(udpcap_socket2.isClosed());

                                      break;
                                    }

                                    // Check if the received datagram is valid and contains "224.0.0.1" or "224.0.0.2"
                                    ASSERT_TRUE(std::string(received_datagram.data(), received_datagram.size()) == "224.0.0.1" 
                                              || std::string(received_datagram.data(), received_datagram.size()) == "224.0.0.2");
                                    received_messages2++;
                                  }
                                });

  // Send the multicast message to 224.0.0.1
  {
    const asio::ip::udp::endpoint endpoint(asio::ip::make_address("224.0.0.1"), 14000);
    std::string buffer_string = "224.0.0.1";
    asio_socket.send_to(asio::buffer(buffer_string), endpoint);
  }

  // Send the multicast message to 224.0.0.2
  {
    const asio::ip::udp::endpoint endpoint(asio::ip::make_address("224.0.0.2"), 14000);
    std::string buffer_string = "224.0.0.2";
    asio_socket.send_to(asio::buffer(buffer_string), endpoint);
  }

  // Wait for received_messages1 to be 1 and received_messages2 to be 2
  received_messages1.wait_for([](int value) { return value >= 1; }, std::chrono::milliseconds(500));
  received_messages2.wait_for([](int value) { return value >= 2; }, std::chrono::milliseconds(500));

  // Check if the received message counters
  ASSERT_EQ(received_messages1.get(), 1) << "Make sure that your FIREWALL is DISABLED!!!";
  ASSERT_EQ(received_messages2.get(), 2) << "Make sure that your FIREWALL is DISABLED!!!";

  // Close the sockets
  asio_socket.close();
  udpcap_socket1.close();
  udpcap_socket2.close();

  // Join the threads
  receive_thread1.join();
  receive_thread2.join();
}

// Create and destroy a bound many Udpcap sockets with a thread waiting for a datagram
TEST(udpcap, ManySockets)
{
  constexpr int       num_udpcap_socket   = 100;
  constexpr char*     ip_address          = "127.0.0.1";
  constexpr uint16_t  port                = 14000;

  // Create an asio socket that sends datagrams to the ip address and port
  asio::io_service      io_service;
  asio::ip::udp::socket asio_socket(io_service, asio::ip::udp::v4());
  asio::ip::udp::endpoint endpoint(asio::ip::make_address(ip_address), port);
  asio_socket.connect(endpoint);

  // Thread that constantly pushes datagrams via the asio socket
  std::thread send_thread([&asio_socket]()
                          {
                            std::string buffer_string = "Hello World";
                            while(true)
                            {
                              asio::error_code ec;
                              asio_socket.send(asio::buffer(buffer_string), 0, ec);
                              if (ec)
                              {
                                break;
                              }
                            }
                          });

  // Create num_udpcap_socket udpcap sockets
  std::vector<Udpcap::UdpcapSocket> udpcap_sockets;
  std::vector<std::thread>          receive_threads;

  // Reserve space for the sockets
  udpcap_sockets.reserve(num_udpcap_socket);

  for (int i = 0; i < num_udpcap_socket; i++)
  {
    udpcap_sockets.emplace_back();
    ASSERT_TRUE(udpcap_sockets.back().isValid());
    const bool success = udpcap_sockets.back().bind(Udpcap::HostAddress(ip_address), port);
    ASSERT_TRUE(success);

    // Create a receive thread that constantly receives datagrams
    receive_threads.emplace_back([&udpcap_sockets, i]()
                                  {
                                    while (true)
                                    {
                                      // Initialize variables for the sender's address and port
                                      Udpcap::HostAddress sender_address;
                                      uint16_t            sender_port(0);
                                      Udpcap::Error       error = Udpcap::Error::ErrorCode::GENERIC_ERROR;
                                      
                                      // Allocate buffer with max udp datagram size
                                      std::vector<char> received_datagram;
                                      received_datagram.resize(65536);
                                      
                                      // blocking receive
                                      const size_t received_bytes = udpcap_sockets[i].receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error);
                                      received_datagram.resize(received_bytes);

                                      if (error)
                                      {
                                        // Indicates that somebody closed the socket
                                        ASSERT_EQ(error, Udpcap::Error(Udpcap::Error::ErrorCode::SOCKET_CLOSED));
    
                                        // Check that the socket is closed
                                        ASSERT_TRUE(udpcap_sockets[i].isClosed());
    
                                        break;
                                      }
                                      else
                                      {
                                        // Check if the received datagram is valid and contains "Hello World"
                                        ASSERT_FALSE(received_datagram.empty());
                                        ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), "Hello World");
                                      }
                                    }
                                  });
  }

  // wait 10ms
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

   
  // Close the sockets
  for (auto& udpcap_socket : udpcap_sockets)
  {
    udpcap_socket.close();
  }
    
  // Join the threads
  for (auto& receive_thread : receive_threads)
  {
    receive_thread.join();
  }
    
  // Close the asio socket
  asio_socket.close();
    
  // Join the send thread
  send_thread.join();
}

// Create many Udpcap multicast sockets and join / leave multicast groups while receiving datagrams
TEST(udpcap, ManyMulticastSockets)
{
  constexpr int       num_udpcap_socket   = 10;
  constexpr int       num_test_loops      = 5;
  constexpr char*     multicast_group_1   = "225.0.0.1";
  constexpr char*     multicast_group_2   = "225.0.0.2";
  constexpr uint16_t  port                = 14000;

  // Create asio sockets to send datagrams to the multicast groups
  asio::io_service      io_service;
  asio::ip::udp::socket asio_socket1(io_service, asio::ip::udp::v4());
  asio::ip::udp::socket asio_socket2(io_service, asio::ip::udp::v4());
  asio::ip::udp::endpoint endpoint1(asio::ip::make_address(multicast_group_1), port);
  asio::ip::udp::endpoint endpoint2(asio::ip::make_address(multicast_group_2), port);
  asio_socket1.set_option(asio::ip::multicast::hops(1));
  asio_socket2.set_option(asio::ip::multicast::hops(1));
  asio_socket1.set_option(asio::ip::multicast::enable_loopback(true));
  asio_socket2.set_option(asio::ip::multicast::enable_loopback(true));

  asio_socket1.connect(endpoint1);
  asio_socket2.connect(endpoint2);

  // Thread that constantly pushes datagrams via the asio sockets
  std::thread send_thread1([&asio_socket1]()
                            {
                              std::string buffer_string = "Hello World";
                              while (true)
                              {
                                asio::error_code ec;
                                asio_socket1.send(asio::buffer(buffer_string), 0, ec);
                                if (ec)
                                {
                                  break;
                                }
                              }
                            });

  std::thread send_thread2([&asio_socket2]()
                            {
                              std::string buffer_string = "Hello World";
                              while (true)
                              {
                                asio::error_code ec;
                                asio_socket2.send(asio::buffer(buffer_string), 0, ec);
                                if (ec)
                                {
                                  break;
                                }
                              }
                            });

  // Create num_udpcap_socket udpcap sockets
  std::vector<Udpcap::UdpcapSocket> udpcap_sockets;
  std::vector<std::thread>          receive_threads;

  // Reserve space for the sockets
  udpcap_sockets.reserve(num_udpcap_socket);

  for (int i = 0; i < num_udpcap_socket; i++)
  {
    udpcap_sockets.emplace_back();
    ASSERT_TRUE(udpcap_sockets.back().isValid());
    const bool success = udpcap_sockets.back().bind(Udpcap::HostAddress::Any(), port);
    ASSERT_TRUE(success);
    udpcap_sockets.back().setMulticastLoopbackEnabled(true);
    
    // Create a receive thread that constantly receives datagrams
    receive_threads.emplace_back([&udpcap_sockets, i, multicast_group_1, multicast_group_2]()
                                {
                                  while (true)
                                  {
                                    // Initialize variables for the sender's address and port
                                    Udpcap::HostAddress sender_address;
                                    uint16_t            sender_port(0);
                                    Udpcap::Error       error = Udpcap::Error::ErrorCode::GENERIC_ERROR;
                                        
                                    // Allocate buffer with max udp datagram size
                                    std::vector<char> received_datagram;
                                    received_datagram.resize(65536);
                                        
                                    // blocking receive
                                    const size_t received_bytes = udpcap_sockets[i].receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error);
                                    received_datagram.resize(received_bytes);
    
                                    if (error)
                                    {
                                      // Indicates that somebody closed the socket
                                      ASSERT_EQ(error, Udpcap::Error(Udpcap::Error::ErrorCode::SOCKET_CLOSED));
        
                                      // Check that the socket is closed
                                      ASSERT_TRUE(udpcap_sockets[i].isClosed());
        
                                      break;
                                    }
                                    else
                                    {
                                      // Check if the received datagram is valid and contains "Hello World"
                                      ASSERT_FALSE(received_datagram.empty());
                                      ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), "Hello World");
                                    }
                                  }
                                });
  }

  for (int i = 0; i < num_test_loops; i++)
  {
    // Join the multicast group 1
    for (auto& udpcap_socket : udpcap_sockets)
    {
      const bool success = udpcap_socket.joinMulticastGroup(Udpcap::HostAddress(multicast_group_1));
      ASSERT_TRUE(success);
    }

    // Join the multicast group 2
    for (auto& udpcap_socket : udpcap_sockets)
    {
      const bool success = udpcap_socket.joinMulticastGroup(Udpcap::HostAddress(multicast_group_2));
      ASSERT_TRUE(success);
    }

    // Leave the multicast group 1
    for (auto& udpcap_socket : udpcap_sockets)
    {
      const bool success = udpcap_socket.leaveMulticastGroup(Udpcap::HostAddress(multicast_group_1));
      ASSERT_TRUE(success);
    }

    // Leave the multicast group 2
    for (auto& udpcap_socket : udpcap_sockets)
    {
      const bool success = udpcap_socket.leaveMulticastGroup(Udpcap::HostAddress(multicast_group_2));
      ASSERT_TRUE(success);
    }
  }

  // Close the sockets
  for (auto& udpcap_socket : udpcap_sockets)
  {
    udpcap_socket.close();
  }

  // Join the threads
  for (auto& receive_thread : receive_threads)
  {
    receive_thread.join();
  }

  // Close the asio sockets
  asio_socket1.close();
  asio_socket2.close();

  // Join the send threads
  send_thread1.join();
  send_thread2.join();
}
