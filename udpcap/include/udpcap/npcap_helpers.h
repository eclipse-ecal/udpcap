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

#include <mutex>
#include <string>

// IWYU pragma: begin_exports
#include <udpcap/udpcap_export.h>
// IWYU pragma: end_exports

namespace Udpcap
{
  static std::mutex pcap_compile_mutex; // pcap_compile is not thread safe, so we need a global mutex

  /**
   * @brief Initializes Npcap, if not done already. Must be called before calling any native npcap methods.
   *
   * This method initialized Npcap and must be called at least once before
   * calling any ncap functions.
   * As it always returns true when npcap has been intialized successfully, it
   * can also be used to check whether npcap is available and working properly.
   *
   * If this function returns true, npcap should work.
   *
   * @return True if npcap is working
   */
  UDPCAP_EXPORT bool Initialize();
    
  /**
   * @brief Checks whether npcap has been initialized successfully
   * @return true if npcap has been initialized successfully
   */
  UDPCAP_EXPORT bool IsInitialized();

  /**
   * @brief Gets the device name of the npcap loopback device as read from the registry
   *
   * The device name has the form: \device\npcap_{6DBF8591-55F9-4DEF-A317-54B9563A42E3}
   * If a modern NPCAP version has been installed without legacy loopback support,
   * The device name will always be \device\npf_loopback
   *
   * @return The name of the loopback device
   */
  UDPCAP_EXPORT std::string GetLoopbackDeviceName();

  /**
   * @brief Gets the UUID of the npcap loopback device as read from the registry
   *
   * The UUID has the form 6DBF8591-55F9-4DEF-A317-54B9563A42E3
   *
   * @return The UUID of the loopback device (or "", if the device could not be determined)
   */
  UDPCAP_EXPORT std::string GetLoopbackDeviceUuidString();

  /**
   * @brief Checks for a given device name, if it is the npcap loopback device
   *
   * @param device_name  The entire device name
   *
   * @return True, if the device matches the NPCAP loopback device.
   */
  UDPCAP_EXPORT bool IsLoopbackDevice(const std::string& device_name);

  /**
   * @brief Returns a human readible status message.
   *
   * This message is intended to be displayed in a graphical user interface.
   * For terminal based applications it is not needed, as the messages are also
   * printed to stderr.
   *
   * @return The Udpcap status as human-readible text (may be multi-line)
   */
  UDPCAP_EXPORT std::string GetHumanReadibleErrorText();
}
