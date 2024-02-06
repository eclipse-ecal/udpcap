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

#include <gtest/gtest.h>

#include <udpcap/udpcap_socket.h>
#include <asio.hpp>

#include <thread>

#include "atomic_signalable.h"

TEST(udpcap, RAII)
{
  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  // Delete the socket
}

TEST(udpcap, RAIIWithClose)
{
  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  // bind the socket
  bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
  ASSERT_TRUE(success);

  // Close the socket
  udpcap_socket.close();
}

TEST(udpcap, RAIIWithSomebodyWaiting)
{
  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  // bind the socket
  bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
  ASSERT_TRUE(success);
      
  // Blocking receive a datagram
  std::thread receive_thread([&udpcap_socket]()
                            {
                              // Create buffer with max udp datagram size
                              std::vector<char> received_datagram;
                              received_datagram.resize(65536);

                              Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;

                              // blocking receive
                              size_t received_bytes =  udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), 0, error);

                              // Check that we didn't receive any bytes
                              ASSERT_EQ(received_bytes, 0);

                              // TODO: check actual error, which should indicate that the socket is closed
                              ASSERT_TRUE(bool(error));

                            });

  // Close the socket
  udpcap_socket.close();

  // Join the thread
  receive_thread.join();

  // Delete the socket
}

TEST(udpcap, SimpleReceive)
{
  atomic_signalable<int> received_messages(0);

  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  {
    bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
    ASSERT_TRUE(success);
  }

  // Blocking receive a datagram
  std::thread receive_thread([&udpcap_socket, &received_messages]()
                              {
                                // Initialize variables for the sender's address and port
                                Udpcap::HostAddress sender_address;
                                uint16_t            sender_port(0);
                                Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;

                                // Allocate buffer with max udp datagram size
                                std::vector<char> received_datagram;
                                received_datagram.resize(65536);

                                // blocking receive
                                size_t received_bytes = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), &sender_address, &sender_port, error);
                                received_datagram.resize(received_bytes);

                                // No error must have occurred
                                ASSERT_FALSE(bool(error));

                                // Check if the received datagram is valid and contains "Hello World"
                                ASSERT_FALSE(received_datagram.empty());
                                ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), "Hello World");

                                received_messages++;
                              });

  // Create an asio UDP sender socket
  asio::io_service io_service;

  const asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 14000);
  asio::ip::udp::socket         asio_socket(io_service, endpoint.protocol());

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
    bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
    ASSERT_TRUE(success);
  }

  // Receive datagrams in a separate thread
  std::thread receive_thread([&udpcap_socket, &received_messages, num_packages_to_send]()
                              {
                                while (true)
                                {
                                  Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;

                                  // Allocate buffer with max udp datagram size
                                  std::vector<char> received_datagram;
                                  received_datagram.resize(65536);

                                  // blocking receive
                                  size_t received_bytes = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), error);

                                  if (error)
                                  {
                                    // TODO: Check that actual error reason

                                    // Indicates that somebody closed the socket
                                    ASSERT_EQ(received_messages.get(), num_packages_to_send);
                                    break;
                                  }

                                  received_datagram.resize(received_bytes);

                                  // Check if the received datagram is valid and contains "Hello World"
                                  ASSERT_FALSE(received_datagram.empty());
                                  ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), "Hello World");

                                  received_messages++;
                                }
                              });

  // Create an asio UDP sender socket
  asio::io_service io_service;

  const asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 14000);
  asio::ip::udp::socket         asio_socket(io_service, endpoint.protocol());

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

TEST(udpcap, SimpleReceiveWithBuffer)
{
  atomic_signalable<int> received_messages(0);

  // Create a udpcap socket
  Udpcap::UdpcapSocket udpcap_socket;
  ASSERT_TRUE(udpcap_socket.isValid());

  {
    bool success = udpcap_socket.bind(Udpcap::HostAddress::LocalHost(), 14000);
    ASSERT_TRUE(success);
  }

  // Create an asio UDP sender socket
  asio::io_service io_service;

  const asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 14000);
  asio::ip::udp::socket         asio_socket(io_service, endpoint.protocol());

  // Send "Hello World" without currently polling the socket
  std::string buffer_string = "Hello World";
  asio_socket.send_to(asio::buffer(buffer_string), endpoint);

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Receive the datagram
  std::thread receive_thread([&udpcap_socket, &received_messages]()
                              {
                                Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;

                                // Create buffer with max udp datagram size
                                std::vector<char> received_datagram;
                                received_datagram.resize(65536);

                                received_datagram.resize(udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), error));

                                // No error must have occurred
                                ASSERT_FALSE(bool(error));

                                // Check if the received datagram is valid and contains "Hello World"
                                ASSERT_FALSE(received_datagram.empty());
                                ASSERT_EQ(std::string(received_datagram.data(), received_datagram.size()), "Hello World");

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

TEST(udpcap, DelayedPackageReceiveMultiplePackages)
{
  constexpr int num_packages_to_send = 100; // TODO: increase
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
    bool success = udpcap_socket.bind(Udpcap::HostAddress::Any(), 14000);
    ASSERT_TRUE(success);
  }

  // Receive datagrams in a separate thread
  std::thread receive_thread([&udpcap_socket, &received_messages, num_packages_to_send, size_per_package, receive_delay]()
                              {
                                while (true)
                                {
                                  // Initialize variables for the sender's address and port
                                  Udpcap::HostAddress sender_address;
                                  uint16_t            sender_port(0);

                                  Udpcap::Error error = Udpcap::Error::ErrorCode::GENERIC_ERROR;

                                  std::vector<char> received_datagram;
                                  received_datagram.resize(65536);

                                  size_t bytes_received = udpcap_socket.receiveDatagram(received_datagram.data(), received_datagram.size(), 0, &sender_address, &sender_port, error);
                                  received_datagram.resize(bytes_received);

                                  if (error)
                                  {
                                    // Indicates that somebody closed the socket
                                    ASSERT_EQ(received_messages.get(), num_packages_to_send);
                                    break;
                                  }

                                  // Check if the received datagram is valid and contains "Hello World"
                                  ASSERT_EQ(received_datagram.size(), size_per_package);
                                  received_messages++;

                                  // Wait a bit, so we force the udpcap socket to buffer the datagrams
                                  std::this_thread::sleep_for(receive_delay);
                                }
                              });

  // Create an asio UDP sender socket
  asio::io_service io_service;

  const asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 14000);
  asio::ip::udp::socket         asio_socket(io_service, endpoint.protocol());

  // Capture the start time
  auto start_time = std::chrono::steady_clock::now();

  // Send the buffers
  for (int i = 0; i < num_packages_to_send; i++)
  {
    asio_socket.send_to(asio::buffer(buffer), endpoint);
  }

  // Wait some time for the receive thread to finish
  received_messages.wait_for([num_packages_to_send](int value) { return value >= num_packages_to_send; }, receive_delay * num_packages_to_send + std::chrono::milliseconds(1000));

  // Check if the received message counter is equal to the sent messages
  ASSERT_EQ(received_messages.get(), num_packages_to_send);

  // Capture the end time
  auto end_time = std::chrono::steady_clock::now();

  // TODO: check the entire delay

  asio_socket.close();
  udpcap_socket.close();

  receive_thread.join();
}


// TODO: Write a test that tests the Source Address and Source Port

// TODO: Write a test that tests the timeout

// TODO: test isclosed function