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

#include <udpcap/host_address.h>
#include <udpcap/error.h>

#include <chrono>
#include <deque>
#include <memory>
#include <set>
#include <shared_mutex>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <pcap.h>           // Pcap API

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4800 4200)
#endif // _MSC_VER
#include <Packet.h>         // Pcap++
#include <IPv4Layer.h>      // Pcap++ IPv4
#include <UdpLayer.h>       // Pcap++ UDP
#ifdef _MSC_VER
#pragma warning( pop )
#endif // _MSC_VER

#include "ip_reassembly.h"

namespace Udpcap
{
  class UdpcapSocketPrivate
  {
  //////////////////////////////////////////
  //// Helper Structs
  //////////////////////////////////////////
  private:
    struct PcapDev
    {
      PcapDev(pcap_t* pcap_handle, bool is_loopback, const std::string& device_name)
        : pcap_handle_(pcap_handle)
        , is_loopback_   (is_loopback)
        , device_name_(device_name)
      {}
      pcap_t*      pcap_handle_;
      bool         is_loopback_;
      std::string  device_name_;
    };

    struct CallbackArgsRawPtr
    {
      CallbackArgsRawPtr(char* destination_buffer, size_t destination_buffer_size, HostAddress* source_address, uint16_t* source_port, uint16_t bound_port, pcpp::LinkLayerType link_type)
        : destination_buffer_     (destination_buffer)
        , destination_buffer_size_(destination_buffer_size)
        , bytes_copied_           (0)
        , source_address_         (source_address)
        , source_port_            (source_port)
        , success_                (false)
        , link_type_              (link_type)
        , bound_port_             (bound_port)
        , ip_reassembly_          (nullptr)
      {}
      char* const               destination_buffer_;
      const size_t              destination_buffer_size_;
      size_t                    bytes_copied_;
      HostAddress* const        source_address_;
      uint16_t* const           source_port_;
      bool                      success_;

      pcpp::LinkLayerType       link_type_;
      const uint16_t            bound_port_;
      Udpcap::IpReassembly*     ip_reassembly_;
    };

  //////////////////////////////////////////
  //// Socket API
  //////////////////////////////////////////
  public:
    static const int MAX_PACKET_SIZE = 65536; // Npcap Doc: A snapshot length of 65535 should be sufficient, on most if not all networks, to capture all the data available from the packet. 

    UdpcapSocketPrivate();
    ~UdpcapSocketPrivate();

    // Copy
    UdpcapSocketPrivate(UdpcapSocketPrivate const&)             = delete;
    UdpcapSocketPrivate& operator= (UdpcapSocketPrivate const&) = delete;

    // Move
    UdpcapSocketPrivate& operator=(UdpcapSocketPrivate&&)      = default;
    UdpcapSocketPrivate(UdpcapSocketPrivate&&)                 = default;

    bool isValid() const;

    bool bind(const HostAddress& local_address, uint16_t local_port);
    bool isBound() const;

    HostAddress localAddress() const;
    uint16_t localPort() const;

    bool setReceiveBufferSize(int buffer_size);

    size_t receiveDatagram(char*            data
                          , size_t          max_len
                          , long long       timeout_ms
                          , HostAddress*    source_address
                          , uint16_t*       source_port
                          , Udpcap::Error&  error);

    bool joinMulticastGroup(const HostAddress& group_address);
    bool leaveMulticastGroup(const HostAddress& group_address);

    void setMulticastLoopbackEnabled(bool enabled);
    bool isMulticastLoopbackEnabled() const;

    void close();
    bool isClosed() const;

  //////////////////////////////////////////
  //// Internal
  //////////////////////////////////////////
  private:
    static std::pair<std::string, std::string> getDeviceByIp(const HostAddress& ip);
    static std::vector<std::pair<std::string, std::string>> getAllDevices();

    static std::string getMac(pcap_t* pcap_handle);

    bool openPcapDevice_nolock(const std::string& device_name);

    std::string createFilterString(PcapDev& pcap_dev) const;
    void updateCaptureFilter(PcapDev& pcap_dev);
    void updateAllCaptureFilters();

    void kickstartLoopbackMulticast() const;

    // Callbacks
    static void PacketHandlerRawPtr(unsigned char* param, const struct pcap_pkthdr* header, const unsigned char* pkt_data);
    static void FillCallbackArgsRawPtr(CallbackArgsRawPtr* callback_args, const pcpp::IPv4Layer* ip_layer, const pcpp::UdpLayer* udp_layer);

  private:
    bool        is_valid_;                                                      /**< If the socket is valid and ready to use (e.g. npcap was initialized successfully) */

    bool        bound_state_;                                                   /**< Whether the socket is in bound state and ready to receive data */
    HostAddress bound_address_;                                                 /**< Local interface address used to read data from */
    uint16_t    bound_port_;                                                    /**< Local port to read data from */

    std::set<HostAddress> multicast_groups_;
    bool                  multicast_loopback_enabled_;                          /**< Winsocks style IP_MULTICAST_LOOP: if enabled, the socket can receive loopback multicast packages */

    mutable std::shared_mutex       pcap_devices_lists_mutex_;                  /**< Mutex to protect the pcap_devices_, pcap_win32_handles_, pcap_devices_ip_reassembly_ lists. Only the lists, not the content. */
    mutable std::mutex              pcap_devices_callback_mutex_;               /**< Mutex to protect the pcap_devices during a callback AND the pcap_devices_closed variable. While a callback is running, the pcap_devices MUST NOT be closed. */
    bool                            pcap_devices_closed_;                       /**< Tells whether we have already closed the socket. */
    std::vector<PcapDev>            pcap_devices_;                              /**< List of open PcapDevices */
    std::vector<HANDLE>             pcap_win32_handles_;                        /**< Native Win32 handles to wait for data on the PCAP Devices. The List is in sync with pcap_devices. */
    std::vector<std::unique_ptr<Udpcap::IpReassembly>> pcap_devices_ip_reassembly_;          /**< IP Reassembly for fragmented IP traffic. The list is in sync with the pcap_devices. */

    int                  receive_buffer_size_;
  };
}
