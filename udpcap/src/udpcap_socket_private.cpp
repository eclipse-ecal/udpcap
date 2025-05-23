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

#include "udpcap_socket_private.h"

#include <udpcap/error.h>
#include <udpcap/host_address.h>
#include <udpcap/npcap_helpers.h>

#include "ip_reassembly.h"
#include "log_debug.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <ntddndis.h>       // User-space defines for NDIS driver communication

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <asio.hpp> // IWYU pragma: keep

namespace Udpcap
{
  //////////////////////////////////////////
  //// Socket API
  //////////////////////////////////////////

  UdpcapSocketPrivate::UdpcapSocketPrivate()
    : is_valid_                  (Udpcap::Initialize())
    , bound_state_               (false)
    , bound_port_                (0)
    , multicast_loopback_enabled_(true)
    , receive_buffer_size_       (-1)
    , pcap_devices_closed_       (false)
  {
  }

  UdpcapSocketPrivate::~UdpcapSocketPrivate()
  {
    close();
  }

  bool UdpcapSocketPrivate::isValid() const
  {
    return is_valid_;
  }


  bool UdpcapSocketPrivate::bind(const HostAddress& local_address, uint16_t local_port)
  {
    if (!is_valid_)
    {
      // Invalid socket, cannot bind => fail!
      LOG_DEBUG("Bind error: Socket is invalid");
      return false;
    }

    if (bound_state_)
    {
      // Already bound => fail!
      LOG_DEBUG("Bind error: Socket is already in bound state");
      return false;
    }

    if (!local_address.isValid())
    {
      // Invalid address => fail!
      LOG_DEBUG("Bind error: Host address is invalid");
      return false;
    }


    // Valid address => Try to bind to address!
    
    const std::unique_lock<std::shared_mutex> pcap_devices_lists_lock(pcap_devices_lists_mutex_);

    if (local_address.isLoopback())
    {
      // Bind to localhost (We cannot find it by IP 127.0.0.1, as that IP is technically not even assignable to the loopback adapter).
      LOG_DEBUG(std::string("Opening Loopback device ") + GetLoopbackDeviceName());

      if (!openPcapDevice_nolock(GetLoopbackDeviceName()))
      {
        LOG_DEBUG(std::string("Bind error: Unable to bind to ") + GetLoopbackDeviceName());
        return false;
      }
    }
    else if (local_address == HostAddress::Any())
    {
      // Bind to all adapters
      auto devices = getAllDevices();

      if (devices.empty())
      {
        LOG_DEBUG("Bind error: No devices found");
        return false;
      }

      for (const auto& dev : devices)
      {
        LOG_DEBUG(std::string("Opening ") + dev.first + " (" + dev.second + ")");

        if (!openPcapDevice_nolock(dev.first))
        {
          LOG_DEBUG(std::string("Bind error: Unable to bind to ") + dev.first);
        }
      }
    }
    else
    {
      // Bind to adapter specified by the IP address
      auto dev = getDeviceByIp(local_address);

      if (dev.first.empty())
      {
        LOG_DEBUG("Bind error: No local device with address " + local_address.toString());
        return false;
      }
      
      LOG_DEBUG(std::string("Opening ") + dev.first + " (" + dev.second + ")");

      if (!openPcapDevice_nolock(dev.first))
      {
        LOG_DEBUG(std::string("Bind error: Unable to bind to ") + dev.first);
        return false;
      }

      // Also open loopback adapter. We always have to expect the local machine sending data to its own IP address.
      LOG_DEBUG(std::string("Opening Loopback device ") + GetLoopbackDeviceName());

      if (!openPcapDevice_nolock(GetLoopbackDeviceName()))
      {
        LOG_DEBUG(std::string("Bind error: Unable to open ") + GetLoopbackDeviceName());
        return false;
      }
    }
    
    bound_address_       = local_address;
    bound_port_          = local_port;
    bound_state_         = true;
    pcap_devices_closed_ = false;

    for (auto& pcap_dev : pcap_devices_)
    {
      updateCaptureFilter(pcap_dev);
    }

    return true;
  }

  bool UdpcapSocketPrivate::isBound() const
  {
    return bound_state_;
  }

  HostAddress UdpcapSocketPrivate::localAddress() const
  {
    return bound_address_;
  }

  uint16_t UdpcapSocketPrivate::localPort() const
  {
    return bound_port_;
  }

  bool UdpcapSocketPrivate::setReceiveBufferSize(int buffer_size)
  {
    if (!is_valid_)
    {
      // Invalid socket, cannot bind => fail!
      LOG_DEBUG("Set Receive Buffer Size error: Socket is invalid");
      return false;
    }

    if (bound_state_)
    {
      // Not bound => fail!
      LOG_DEBUG("Set Receive Buffer Size error: Socket is already bound");
      return false;
    }

    if (buffer_size < MAX_PACKET_SIZE)
    {
      // Not bound => fail!
      LOG_DEBUG("Set Receive Buffer Size error: Buffer size is smaller than the maximum expected packet size (" + std::to_string(MAX_PACKET_SIZE) + ")");
      return false;
    }

    receive_buffer_size_ = buffer_size;

    return true;
  }

  size_t UdpcapSocketPrivate::receiveDatagram(char*           data
                                            , size_t          max_len
                                            , long long       timeout_ms
                                            , HostAddress*    source_address
                                            , uint16_t*       source_port
                                            , Udpcap::Error&  error)
  {
    // calculate until when to wait. If timeout_ms is 0 or smaller, we will wait forever.
    std::chrono::steady_clock::time_point wait_until;
    if (timeout_ms < 0)
    {
      wait_until = std::chrono::steady_clock::time_point::max();
    }
    else
    {
      wait_until = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    }

    if (!is_valid_)
    {
      // Invalid socket, cannot bind => fail!
      LOG_DEBUG("Receive error: Socket is invalid");
      error = Udpcap::Error::NPCAP_NOT_INITIALIZED;
      return 0;
    }

    // Check all devices for data
    {
      // Variable to store the result
      pcap_pkthdr*  packet_header (nullptr);
      const u_char* packet_data   (nullptr);

      // Lock the lists of open pcap devices in read-mode. We may use the handles, but not modify the lists themselfes.
      const std::shared_lock<std::shared_mutex> pcap_devices_list_lock(pcap_devices_lists_mutex_);

      // Check for data on pcap devices until we are either out of time or have
      // received a datagaram. A datagram may consist of multiple packaets in
      // case of IP Fragmentation.
      while (true)
      {
        bool received_any_data = false;

        {
          // Lock the callback lock. While the callback is running, we cannot close the pcap handle, as that may invalidate the data pointer.
          const std::lock_guard<std::mutex> pcap_devices_callback_lock(pcap_devices_callback_mutex_);

          // Check if the socket is closed and return an error
          if (pcap_devices_closed_)
          {
            error = Udpcap::Error::SOCKET_CLOSED;
            return 0;
          }
    
          // Check if the socket is bound and return an error
          if (!bound_state_)
          {
            // Not bound => fail!
            LOG_DEBUG("Receive error: Socket is not bound");
            error = Udpcap::Error::NOT_BOUND;
            return 0;
          }

          // Iterate through all devices and check if they have data. There is
          // no other API (that I know of) to check whether data is available on
          // a PCAP device other than trying to claim it. There is a very valid
          // possibility that no device will have any data available. In that
          // case, we use the native Win32 event handles to wait for new data
          // becoming available. We however cannot do that here before trying to
          // receive the data, as waiting on the event would clear the event
          // state and we don't have information about the amount of data being
          // availabe (e.g. there are 2 packets available, but the event is
          // cleared after we waited for the first one).
          for (const auto& pcap_dev : pcap_devices_)
          {
            CallbackArgsRawPtr callback_args(data, max_len, source_address, source_port, bound_port_, pcpp::LinkLayerType::LINKTYPE_NULL);

            const int pcap_next_packet_errorcode = pcap_next_ex(pcap_dev.pcap_handle_, &packet_header, &packet_data);

            // Possible return values:
            //  1: Success! We received a packet.
            //  0: Timeout. As we set the handle to non-blocking mode, this always happens if no packet is available.
            //  PCAP_ERROR_NOT_ACTIVATED: The pcap handle is not activated. This should never happen, as we only use activated handles.
            //  PCAP_ERROR: An error occured. Details can be retrieved using pcap_geterr() or printed to the console using pcap_perror().

            if (pcap_next_packet_errorcode == 1)
            {
              received_any_data = true;

              // Success! We received a packet. Call the packet handler, which
              // also handles IP reassembly and sets the success variable, if we
              // successfully received an entire UDP datagram.
              PacketHandlerRawPtr(reinterpret_cast<unsigned char*>(&callback_args), packet_header, packet_data);

              if (callback_args.success_)
              {
                // Only return datagram if we successfully received a packet. Otherwise, we will continue receiving data, if there is time left.
                error = Udpcap::Error::OK;
                return callback_args.bytes_copied_;
              }
            }
            else if (pcap_next_packet_errorcode == 0)
            {
              // Timeout. No packet available. We will continue receiving data, if there is time left.
              continue;
            }
            else if (pcap_next_packet_errorcode == PCAP_ERROR_NOT_ACTIVATED)
            {
              // This should never happen, as we only use activated handles.
              error = Udpcap::Error(Udpcap::Error::NOT_BOUND, "Internal error: PCAP handle not activated");
              LOG_DEBUG(error.ToString()); // This should never happen in a proper application
              return 0;
            }
            else if (pcap_next_packet_errorcode == PCAP_ERROR)
            {
              // An error occured. Details can be retrieved using pcap_geterr() or printed to the console using pcap_perror().
              error = Udpcap::Error(Udpcap::Error::GENERIC_ERROR, pcap_geterr(pcap_dev.pcap_handle_));
              LOG_DEBUG(error.ToString());
              return 0;
            }
            else
            {
              // This should never happen according to the documentation.
              error = Udpcap::Error(Udpcap::Error::GENERIC_ERROR, "Internal error: Unknown error code " + std::to_string(pcap_next_packet_errorcode));
              LOG_DEBUG(error.ToString()); // This should never happen in a proper application
              return 0;
            }
          }
        }

        // Use WaitForMultipleObjects in order to wait for data on the pcap
        // devices. Only wait for data, if we haven't received any data in the
        // last loop. The Win32 event will be resetted after we got notified,
        // regardless of the amount of packets that are in the buffer. Thus, we
        // cannot use the event to always check / wait for new data, as there
        // may still be data left in the buffer without the event being set.
        if (!received_any_data)
        {
          // Check if we are out of time and return an error if so.
          auto now = std::chrono::steady_clock::now();
          if (now >= wait_until)
          {
            error = Udpcap::Error::TIMEOUT;
            return 0;
          }

          // If we are not out of time, we calculate how many milliseconds we are allowed to wait for new data.
          unsigned long remaining_time_to_wait_ms = 0;
          const bool wait_forever = (timeout_ms < 0); // Original parameter "timeout_ms" is negative if we want to wait forever
          if (wait_forever)
          {
            remaining_time_to_wait_ms = INFINITE;
          }
          else
          {
            remaining_time_to_wait_ms = static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(wait_until - now).count());
          }
          
          DWORD num_handles = static_cast<DWORD>(pcap_win32_handles_.size());
          if (num_handles > MAXIMUM_WAIT_OBJECTS)
          {
            LOG_DEBUG("WARNING: Too many open Adapters. " + std::to_string(num_handles) + " adapters are open, only " + std::to_string(MAXIMUM_WAIT_OBJECTS) + " are supported.");
            num_handles = MAXIMUM_WAIT_OBJECTS;
          }

          const DWORD wait_result = WaitForMultipleObjects(num_handles, pcap_win32_handles_.data(), static_cast<BOOL>(false), remaining_time_to_wait_ms);

          if ((wait_result >= WAIT_OBJECT_0) && wait_result <= (WAIT_OBJECT_0 + num_handles - 1))
          {
            // SUCCESS! Some event is notified! We could actually check which
            // event it is, in order to read data from that specific event. But
            // it is way easier to just let the code above run again and check
            // all pcap devices for data.
            continue;
          }
          else if ((wait_result >= WAIT_ABANDONED_0) && wait_result <= (WAIT_ABANDONED_0 + num_handles - 1))
          {
            error = Udpcap::Error(Udpcap::Error::GENERIC_ERROR, "Internal error \"WAIT_ABANDONED\" while waiting for data: " + std::system_category().message(GetLastError()));
            LOG_DEBUG(error.ToString()); // This should never happen in a proper application
          }
          else if (wait_result == WAIT_TIMEOUT)
          {
            //LOG_DEBUG("Receive error: WAIT_TIMEOUT");
            error = Udpcap::Error::TIMEOUT;
            return 0;
          }
          else if (wait_result == WAIT_FAILED)
          {
            // This probably indicates a closed socket. But we don't need to
            // check it here, we can simply continue the loop, as the first
            // thing the loop does is checking for a closed socket.
            LOG_DEBUG("Receive error: WAIT_FAILED: " + std::system_category().message(GetLastError()));
            continue;
          }
        }
      }
    }
  }

  bool UdpcapSocketPrivate::joinMulticastGroup(const HostAddress& group_address)
  {
    if (!is_valid_)
    {
      LOG_DEBUG("Join Multicast Group error: Socket invalid");
      return false;
    }

    if (!group_address.isValid())
    {
      LOG_DEBUG("Join Multicast Group error: Address invalid");
      return false;
    }

    if (!group_address.isMulticast())
    {
      LOG_DEBUG("Join Multicast Group error: " + group_address.toString() + " is not a multicast address");
      return false;
    }

    if (!bound_state_)
    {
      LOG_DEBUG("Join Multicast Group error: Sockt is not in bound state");
      return false;
    }

    if (multicast_groups_.find(group_address) != multicast_groups_.end())
    {
      LOG_DEBUG("Join Multicast Group error: Already joined " + group_address.toString());
      return false;
    }

    // Add theg group to the group list
    multicast_groups_.emplace(group_address);

    // Update the capture filters, so the devices will capture the multicast traffic
    updateAllCaptureFilters();

    if (multicast_loopback_enabled_)
    {
      // Trigger the Windows kernel to also send multicast traffic to localhost
      kickstartLoopbackMulticast();
    }

    return true;
  }

  bool UdpcapSocketPrivate::leaveMulticastGroup(const HostAddress& group_address)
  {
    if (!is_valid_)
    {
      LOG_DEBUG("Leave Multicast Group error: Socket invalid");
      return false;
    }

    if (!group_address.isValid())
    {
      LOG_DEBUG("Leave Multicast Group error: Address invalid");
      return false;
    }

    auto group_it = multicast_groups_.find(group_address);
    if (group_it == multicast_groups_.end())
    {
      LOG_DEBUG("Leave Multicast Group error: Not member of " + group_address.toString());
      return false;
    }

    // Remove the group from the group list
    multicast_groups_.erase(group_it);

    // Update all capture filtes
    updateAllCaptureFilters();

    return true;
  }

  void UdpcapSocketPrivate::setMulticastLoopbackEnabled(bool enabled)
  {
    if (multicast_loopback_enabled_ == enabled)
    {
      // Nothing changed
      return;
    }

    multicast_loopback_enabled_ = enabled;

    if (multicast_loopback_enabled_)
    {
      // Trigger the Windows kernel to also send multicast traffic to localhost
      kickstartLoopbackMulticast();
    }

    updateAllCaptureFilters();
  }

  bool UdpcapSocketPrivate::isMulticastLoopbackEnabled() const
  {
    return multicast_loopback_enabled_;
  }

  void UdpcapSocketPrivate::close()
  {
    {
      // Lock the lists of open pcap devices in read-mode. We may use the handles,
      // but not modify the lists themselfes. This is in order to assure that the
      // ReceiveDatagram function still has all pcap devices available after
      // returning from WaitForMultipleObjects.
      const std::shared_lock<std::shared_mutex> pcap_devices_lists_lock(pcap_devices_lists_mutex_);

      {
        // Lock the callback lock. While the callback is running, we cannot close
        // the pcap handle, as that may invalidate the data pointer.
        const std::lock_guard<std::mutex> pcap_devices_callback_lock(pcap_devices_callback_mutex_);
        pcap_devices_closed_ = true;
        for (auto& pcap_dev : pcap_devices_)
        {
          LOG_DEBUG(std::string("Closing ") + pcap_dev.device_name_);
          pcap_close(pcap_dev.pcap_handle_);
        }
      }
    }

    {
      // Lock the lists of open pcap devices in write-mode. We may now modify the lists themselfes.
      const std::unique_lock<std::shared_mutex> pcap_devices_lists_lock(pcap_devices_lists_mutex_);
      pcap_devices_              .clear();
      pcap_win32_handles_        .clear();
      pcap_devices_ip_reassembly_.clear();
    }

    bound_state_ = false;
    bound_port_ = 0;
    bound_address_ = HostAddress::Invalid();
  }

  bool UdpcapSocketPrivate::isClosed() const
  {
    const std::lock_guard<std::mutex> pcap_callback_lock(pcap_devices_callback_mutex_);
    return pcap_devices_closed_;
  }

  //////////////////////////////////////////
  //// Internal
  //////////////////////////////////////////

  std::pair<std::string, std::string> UdpcapSocketPrivate::getDeviceByIp(const HostAddress& ip)
  {
    if (!ip.isValid())
      return{};

    // Retrieve device list
    std::array<char, PCAP_ERRBUF_SIZE> errbuf{};
    pcap_if_t* alldevs_ptr = nullptr;

    if (pcap_findalldevs(&alldevs_ptr, errbuf.data()) == -1)
    {
      fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf.data());
      if (alldevs_ptr != nullptr)
        pcap_freealldevs(alldevs_ptr);
      return{};
    }

    for (pcap_if_t* pcap_dev = alldevs_ptr; pcap_dev != nullptr; pcap_dev = pcap_dev->next)
    {
      // A user may have done something bad like assigning an IPv4 address to
      // the loopback adapter. We don't want to open it in that case. In a real-
      // world szenario this may never happen.
      if (IsLoopbackDevice(pcap_dev->name))
      {
        continue;
      }

      // Iterate through all addresses of the device and check if one of them
      // matches the one we are looking for.
      for (pcap_addr* pcap_dev_addr = pcap_dev->addresses; pcap_dev_addr != nullptr; pcap_dev_addr = pcap_dev_addr->next)
      {
        if (pcap_dev_addr->addr->sa_family == AF_INET)
        {         
          struct sockaddr_in* device_ipv4_addr = reinterpret_cast<struct sockaddr_in *>(pcap_dev_addr->addr);
          if (device_ipv4_addr->sin_addr.s_addr == ip.toInt())
          {
            // The IPv4 address matches!
            pcap_freealldevs(alldevs_ptr);
            return std::make_pair(std::string(pcap_dev->name), std::string(pcap_dev->description));
          }
        }
      }
    }

    pcap_freealldevs(alldevs_ptr);

    // Nothing found => nullptr
    return{};
  }

  std::vector<std::pair<std::string, std::string>> UdpcapSocketPrivate::getAllDevices()
  {
    // Retrieve device list
    std::array<char, PCAP_ERRBUF_SIZE> errbuf{};
    pcap_if_t* alldevs_ptr = nullptr;

    if (pcap_findalldevs(&alldevs_ptr, errbuf.data()) == -1)
    {
      fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf.data());
      if (alldevs_ptr != nullptr)
        pcap_freealldevs(alldevs_ptr);
      return{};
    }

    std::vector<std::pair<std::string, std::string>> alldev_vector;
    for (pcap_if_t* pcap_dev = alldevs_ptr; pcap_dev != nullptr; pcap_dev = pcap_dev->next)
    {
      alldev_vector.emplace_back(std::string(pcap_dev->name), std::string(pcap_dev->description));
    }

    pcap_freealldevs(alldevs_ptr);

    return alldev_vector;
  }

  std::string UdpcapSocketPrivate::getMac(pcap_t* pcap_handle)
  {
    // Check whether the handle actually is an ethernet device
    if (pcap_datalink(pcap_handle) == DLT_EN10MB)
    {
      // Data for the OID Request
      size_t mac_size = 6;
      std::vector<char> mac(mac_size);

      // Send OID-Get-Request to the driver
      if (pcap_oid_get_request(pcap_handle, OID_802_3_CURRENT_ADDRESS, mac.data(), &mac_size) != 0)
      {
        LOG_DEBUG("Error getting MAC address");
        return "";
      }

      // Convert binary mac into human-readble form (we need it this way for the kernel filter)
      std::string mac_string(18, ' ');
      snprintf(&mac_string[0], mac_string.size(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); // NOLINT(readability-container-data-pointer) Reason: I need to write to the string, but the data() pointer is const. Since C++11, the operation is safe, as stings are required to be stored in contiguous memory.
      mac_string.pop_back(); // Remove terminating null char

      return std::move(mac_string);
    }
    else
    {
      // If not on ethernet, we assume that we don't have a MAC
      return "";
    }
  }

  bool UdpcapSocketPrivate::openPcapDevice_nolock(const std::string& device_name)
  {
    std::array<char, PCAP_ERRBUF_SIZE> errbuf{};

    pcap_t* pcap_handle = pcap_create(device_name.c_str(), errbuf.data());

    if (pcap_handle == nullptr)
    {
      fprintf(stderr, "\nUnable to open the adapter: %s\n", errbuf.data());
      return false;
    }

    pcap_set_snaplen(pcap_handle, MAX_PACKET_SIZE);
    pcap_set_promisc(pcap_handle, 1 /*true*/); // We only want Packets destined for this adapter. We are not interested in others.
    pcap_set_immediate_mode(pcap_handle, 1 /*true*/);

    std::array<char, PCAP_ERRBUF_SIZE> pcap_setnonblock_errbuf{};
    pcap_setnonblock(pcap_handle, 1 /*true*/,pcap_setnonblock_errbuf.data());

    if (receive_buffer_size_ > 0)
    {
      pcap_set_buffer_size(pcap_handle, receive_buffer_size_);
    }

    const int errorcode = pcap_activate(pcap_handle);
    switch (errorcode)
    {
    case 0:
      break; // SUCCESS!
    case PCAP_WARNING_PROMISC_NOTSUP:
      pcap_perror(pcap_handle, ("UdpcapSocket WARNING: Device " + device_name + " does not support promiscuous mode").c_str());
      break;
    case PCAP_WARNING:
      pcap_perror(pcap_handle, ("UdpcapSocket WARNING: Device " + device_name).c_str());
      break;
    case PCAP_ERROR_ACTIVATED:
      fprintf(stderr, "%s", ("UdpcapSocket ERROR: Device " + device_name + " already activated").c_str());
      pcap_close(pcap_handle);
      return false;
    case PCAP_ERROR_NO_SUCH_DEVICE:
      pcap_perror(pcap_handle, ("UdpcapSocket ERROR: Device " + device_name + " does not exist").c_str());
      pcap_close(pcap_handle);
      return false;
    case PCAP_ERROR_PERM_DENIED:
      pcap_perror(pcap_handle, ("UdpcapSocket ERROR: Device " + device_name + ": Permissoin denied").c_str());
      pcap_close(pcap_handle);
      return false;
    case PCAP_ERROR_RFMON_NOTSUP:
      fprintf(stderr, "%s", ("UdpcapSocket ERROR: Device " + device_name + ": Does not support monitoring").c_str());
      pcap_close(pcap_handle);
      return false;
    case PCAP_ERROR_IFACE_NOT_UP:
      fprintf(stderr, "%s", ("UdpcapSocket ERROR: Device " + device_name + ": Interface is down").c_str());
      pcap_close(pcap_handle);
      return false;
    case PCAP_ERROR:
      pcap_perror(pcap_handle, ("UdpcapSocket ERROR: Device " + device_name).c_str());
      pcap_close(pcap_handle);
      return false;
    default:
      fprintf(stderr, "%s", ("UdpcapSocket ERROR: Device " + device_name + ": Unknown error").c_str());
      pcap_close(pcap_handle);
      return false;
    }


    const PcapDev pcap_dev(pcap_handle, IsLoopbackDevice(device_name), device_name);
   
    pcap_devices_              .push_back(pcap_dev);
    pcap_win32_handles_        .push_back(pcap_getevent(pcap_handle));
    pcap_devices_ip_reassembly_.emplace_back(std::make_unique<Udpcap::IpReassembly>(std::chrono::seconds(5)));

    return true;
  }

  std::string UdpcapSocketPrivate::createFilterString(PcapDev& pcap_dev) const
  {
    std::stringstream ss;

    // No outgoing packets (determined by MAC, loopback packages don't have an ethernet header)
    if (!pcap_dev.is_loopback_)
    {
      const std::string mac_string = getMac(pcap_dev.pcap_handle_);
      if (!mac_string.empty())
      {
        ss << "not ether src " << mac_string;
        ss << " and ";
      }
    }

    // IP traffic having UDP payload
    ss << "ip and udp";

    // UDP Port or IPv4 fragmented traffic (in IP fragments we cannot see the UDP port, yet)
    ss << " and (udp port " << bound_port_ << " or (ip[6:2] & 0x3fff != 0))";

    // IP
    // Unicast traffic
    ss << " and (((not ip multicast) ";
    if (bound_address_ != HostAddress::Any() && bound_address_ != HostAddress::Broadcast())
    {
      ss << "and (ip dst " << bound_address_.toString() << ")";
    }
    ss << ")";
      
    // Multicast traffic
    if ((!multicast_groups_.empty())
      &&(!pcap_dev.is_loopback_ || multicast_loopback_enabled_))
    {
      ss << " or (ip multicast and (";
      for (auto ip_it = multicast_groups_.begin(); ip_it != multicast_groups_.end(); ip_it++)
      {
        if (ip_it != multicast_groups_.begin())
          ss << " or ";
        ss << "dst " << ip_it->toString();
      }
      ss << "))";
    }

    ss << ")";

    return ss.str();
  }

  void UdpcapSocketPrivate::updateCaptureFilter(PcapDev& pcap_dev)
  {
    // Create new filter
    const std::string filter_string = createFilterString(pcap_dev);

    LOG_DEBUG("Setting filter string: " + filter_string);

    bpf_program filter_program{};

    // Compile the filter
    int pcap_compile_ret(PCAP_ERROR);
    {
      // pcap_compile is not thread safe, so we need a global mutex
      const std::lock_guard<std::mutex> pcap_compile_lock(pcap_compile_mutex);
      pcap_compile_ret = pcap_compile(pcap_dev.pcap_handle_, &filter_program, filter_string.c_str(), 1, PCAP_NETMASK_UNKNOWN);
    }

    if (pcap_compile_ret == PCAP_ERROR)
    {
      pcap_perror(pcap_dev.pcap_handle_, ("UdpcapSocket ERROR: Unable to compile filter \"" + filter_string + "\"").c_str()); // TODO: revise error printing
    }
    else
    {
      // Set the filter
      auto set_filter_error = pcap_setfilter(pcap_dev.pcap_handle_, &filter_program);
      if (set_filter_error == PCAP_ERROR)
      {
        pcap_perror(pcap_dev.pcap_handle_, ("UdpcapSocket ERROR: Unable to set filter \"" + filter_string + "\"").c_str());
      }

      pcap_freecode(&filter_program);
    }
  }

  void UdpcapSocketPrivate::updateAllCaptureFilters()
  {
    for (auto& pcap_dev : pcap_devices_)
    {
      updateCaptureFilter(pcap_dev);
    }
  }

  void UdpcapSocketPrivate::kickstartLoopbackMulticast() const
  {
    constexpr uint16_t kickstart_port = 62000;

    asio::io_context iocontext;
    asio::ip::udp::socket kickstart_socket(iocontext);

    // create socket
    const asio::ip::udp::endpoint listen_endpoint(asio::ip::make_address("0.0.0.0"), kickstart_port);
    {
      asio::error_code ec;
      kickstart_socket.open(listen_endpoint.protocol(), ec);
      if (ec)
      {
        LOG_DEBUG("Failed to open kickstart socket: " + ec.message());
        return;
      }
    }

    // set socket reuse
    {
      asio::error_code ec;
      kickstart_socket.set_option(asio::ip::udp::socket::reuse_address(true), ec);
      if (ec)
      {
        LOG_DEBUG("Failed to set socket reuse of kickstart socket: " + ec.message());
        return;
      }
    }

    // bind socket
    {
      asio::error_code ec;
      kickstart_socket.bind(listen_endpoint, ec);
      if (ec)
      {
        LOG_DEBUG("Failed to bind kickstart socket: " + ec.message());
        return;
      }
    }

    // multicast loopback
    {
      asio::error_code ec;
      kickstart_socket.set_option(asio::ip::multicast::enable_loopback(true), ec);
      if (ec)
      {
        LOG_DEBUG("Failed to set multicast loopback of kickstart socket: " + ec.message());
        return;
      }
    }

    // multicast ttl
    {
      asio::error_code ec;
      kickstart_socket.set_option(asio::ip::multicast::hops(0), ec);
      if (ec)
      {
        LOG_DEBUG("Failed to set multicast ttl of kickstart socket: " + ec.message());
        return;
      }
    }

    // Join all multicast groups
    for (const auto& multicast_group : multicast_groups_)
    {
      const asio::ip::address asio_mc_group = asio::ip::make_address(multicast_group.toString());

      asio::error_code ec;
      kickstart_socket.set_option(asio::ip::multicast::join_group(asio_mc_group), ec);
      if (ec)
      {
        LOG_DEBUG("Failed to join multicast group " + multicast_group.toString() + " with kickstart socket: " + ec.message());
      }
    }

    // Send data to all multicast groups
    for (const auto& multicast_group : multicast_groups_)
    {
      LOG_DEBUG(std::string("Sending loopback kickstart packet to ") + multicast_group.toString() + ":" + std::to_string(kickstart_port));
      const asio::ip::address asio_mc_group = asio::ip::make_address(multicast_group.toString());
      const asio::ip::udp::endpoint send_endpoint(asio_mc_group, kickstart_port);

      {
        asio::error_code ec;
        kickstart_socket.send_to(asio::buffer(static_cast<void*>(nullptr), 0), send_endpoint, 0, ec);
        if (ec)
        {
          LOG_DEBUG("Failed to send kickstart packet to " + multicast_group.toString() + ":" + std::to_string(kickstart_port) + ": " + ec.message());
        }
      }
    }

    // Close the socket
    {
      asio::error_code ec;
      kickstart_socket.close();
      if (ec)
      {
        LOG_DEBUG("Failed to close kickstart socket: " + ec.message());
      }
    }
  }

  void UdpcapSocketPrivate::PacketHandlerRawPtr(unsigned char* param, const struct pcap_pkthdr* header, const unsigned char* pkt_data)
  {
    CallbackArgsRawPtr* callback_args = reinterpret_cast<CallbackArgsRawPtr*>(param);

    pcpp::RawPacket       rawPacket(pkt_data, header->caplen, header->ts, false, callback_args->link_type_);
    const pcpp::Packet    packet(&rawPacket, pcpp::UDP);

    const pcpp::IPv4Layer* ip_layer = packet.getLayerOfType<pcpp::IPv4Layer>();
    const pcpp::UdpLayer*  udp_layer = packet.getLayerOfType<pcpp::UdpLayer>();

    if (ip_layer != nullptr)
    {
      if (ip_layer->isFragment())
      {
        // Handle fragmented IP traffic
        pcpp::IPReassembly::ReassemblyStatus status(pcpp::IPReassembly::ReassemblyStatus::NON_IP_PACKET);

        // Try to reasseble packet
        pcpp::Packet* reassembled_packet = callback_args->ip_reassembly_->processPacket(&rawPacket, status);

        // If we are done reassembling the packet, we return it to the user
        if (reassembled_packet != nullptr)
        {
          const pcpp::Packet re_parsed_packet(reassembled_packet->getRawPacket(), pcpp::UDP);

          const pcpp::IPv4Layer* reassembled_ip_layer = re_parsed_packet.getLayerOfType<pcpp::IPv4Layer>();
          const pcpp::UdpLayer*  reassembled_udp_layer = re_parsed_packet.getLayerOfType<pcpp::UdpLayer>();

          if ((reassembled_ip_layer != nullptr) && (reassembled_udp_layer != nullptr))
            FillCallbackArgsRawPtr(callback_args, reassembled_ip_layer, reassembled_udp_layer);

          delete reassembled_packet; // We need to manually delete the packet pointer
        }
      }
      else if (udp_layer != nullptr)
      {
        // Handle normal IP traffic (un-fragmented)
        FillCallbackArgsRawPtr(callback_args, ip_layer, udp_layer);
      }
    }

  }

  void UdpcapSocketPrivate::FillCallbackArgsRawPtr(CallbackArgsRawPtr* callback_args, const pcpp::IPv4Layer* ip_layer, const pcpp::UdpLayer* udp_layer)
  {
    auto dst_port = ntohs(udp_layer->getUdpHeader()->portDst);

    if (dst_port == callback_args->bound_port_)
    {
      if (callback_args->source_address_ != nullptr)
        *callback_args->source_address_ = HostAddress(ip_layer->getSrcIPv4Address().toInt());

      if (callback_args->source_port_ != nullptr)
        *callback_args->source_port_ = ntohs(udp_layer->getUdpHeader()->portSrc);


      const size_t bytes_to_copy = std::min(callback_args->destination_buffer_size_, udp_layer->getLayerPayloadSize());

      memcpy_s(callback_args->destination_buffer_, callback_args->destination_buffer_size_, udp_layer->getLayerPayload(), bytes_to_copy);
      callback_args->bytes_copied_ = bytes_to_copy;

      callback_args->success_ = true;
    }

  }
}
