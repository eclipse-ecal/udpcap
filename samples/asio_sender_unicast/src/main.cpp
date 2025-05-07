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

#include <asio.hpp>

int main()
{
  //
  // This is a quick-and-dirty sample that uses asio to send UDP data.
  // It does not use Udpcap.
  // 
  // Please do not use this as a reference.
  //

  asio::io_context io_context;

  const asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 14000);
  asio::ip::udp::socket         udp_socket(io_context, endpoint.protocol());

  int counter = 0;
  for(;;)
  {
    std::string buffer_string = "Hello World " + std::to_string(counter);

    std::cout << "Sending data \"" << buffer_string << "\"" << std::endl;
    udp_socket.send_to(asio::buffer(buffer_string), endpoint);
    counter++;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }

  return 0;
}
