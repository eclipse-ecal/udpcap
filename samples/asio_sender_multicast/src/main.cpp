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

#include <asio.hpp>

int main()
{
  //
  // This is a quick-and-dirty sample that uses asio to send multicast data.
  // It does not use Udpcap.
  // 
  // Please do not use this as a reference.
  //

  asio::io_service io_service;

  asio::ip::udp::endpoint endpoint(asio::ip::make_address("239.0.0.1"), 14000);
  asio::ip::udp::socket   upd_socket(io_service, endpoint.protocol());

  // set multicast packet TTL
  {
    asio::ip::multicast::hops ttl(2);
    asio::error_code ec;
    upd_socket.set_option(ttl, ec);
    if (ec)
    {
      std::cerr << "ERROR: Setting TTL failed: " << ec.message() << std::endl;
      return 1;
    }
  }

  // set loopback option
  {
    asio::ip::multicast::enable_loopback loopback(true);
    asio::error_code ec;
    upd_socket.set_option(loopback);
    if (ec)
    {
      std::cerr << "ERROR: Error setting loopback option: " << ec.message() << std::endl;
      return 1;
    }
  }

  int counter = 0;
  for(;;)
  {
    std::string buffer_string = "Hello World " + std::to_string(counter);

    std::cout << "Sending data \"" << buffer_string << "\"" << std::endl;
    upd_socket.send_to(asio::buffer(buffer_string), endpoint);
    counter++;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  return 0;
}
