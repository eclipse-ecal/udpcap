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
#include "udpcap/npcap_helpers.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <codecvt>
#include <cstdio>
#include <iostream>
#include <locale>
#include <mutex>
#include <sstream>
#include <string>
#include <tchar.h>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // IWYU pragma: keep

#include <pcap/pcap.h>

namespace Udpcap
{
  namespace // Private Namespace
  {
    std::mutex npcap_mutex;
    bool is_initialized(false);

    std::string loopback_device_uuid_string;
    bool loopback_device_name_initialized(false);

    std::string human_readible_error_("Npcap has not been initialized, yet");

    bool LoadNpcapDlls()
    {
      std::array<_TCHAR, 512> npcap_dir{};
      UINT len(0);
      len = GetSystemDirectory(npcap_dir.data(), 480);
      if (len == 0) {
        human_readible_error_ = "Error in GetSystemDirectory";
        fprintf(stderr, "Error in GetSystemDirectory: %x", GetLastError());
        return false;
      }
      _tcscat_s(npcap_dir.data(), 512, _T("\\Npcap"));
      if (SetDllDirectory(npcap_dir.data()) == 0) {
        human_readible_error_ = "Error in SetDllDirectory";
        fprintf(stderr, "Error in SetDllDirectory: %x", GetLastError());
        return false;
      }

      if(LoadLibrary("wpcap.dll") == nullptr)
        return false;

      return true;
    }

    LONG GetDWORDRegKey(HKEY hKey, const std::wstring &strValueName, DWORD &nValue, DWORD nDefaultValue)
    {
      nValue = nDefaultValue;
      DWORD dwBufferSize(sizeof(DWORD));
      DWORD nResult(0);
      const LONG nError = ::RegQueryValueExW(hKey,
                                            strValueName.c_str(),
                                            nullptr,
                                            nullptr,
                                            reinterpret_cast<LPBYTE>(&nResult),
                                            &dwBufferSize);
      if (ERROR_SUCCESS == nError)
      {
        nValue = nResult;
      }
      return nError;
    }


    LONG GetBoolRegKey(HKEY hKey, const std::wstring &strValueName, bool &bValue, bool bDefaultValue)
    {
      const DWORD nDefValue((bDefaultValue) ? 1 : 0);
      DWORD nResult(nDefValue);
      const LONG nError = GetDWORDRegKey(hKey, strValueName, nResult, nDefValue);
      if (ERROR_SUCCESS == nError)
      {
        bValue = (nResult != 0);
      }
      return nError;
    }


    LONG GetStringRegKey(HKEY hKey, const std::wstring &strValueName, std::wstring &strValue, const std::wstring &strDefaultValue)
    {
      strValue = strDefaultValue;
      std::array<WCHAR, 512> szBuffer{};
      DWORD dwBufferSize = sizeof(szBuffer);
      ULONG nError(0);
      nError = RegQueryValueExW(hKey, strValueName.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(szBuffer.data()), &dwBufferSize);
      if (ERROR_SUCCESS == nError)
      {
        strValue = szBuffer.data();
      }
      return nError;
    }

    bool LoadLoopbackDeviceNameFromRegistry()
    {
      HKEY hkey = nullptr;
      const LONG error_code = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\npcap\\Parameters", 0, KEY_READ, &hkey);
      if (error_code != 0)
      {
        human_readible_error_ = "NPCAP doesn't seem to be installed. Please download and install Npcap from https://nmap.org/npcap/#download";
        std::cerr << "Udpcap ERROR: " << human_readible_error_ << std::endl;
        return false;
      }

      bool loopback_supported = false;
      GetBoolRegKey(hkey, L"LoopbackSupport", loopback_supported, false);

      if (!loopback_supported)
      {
        human_readible_error_ = "NPCAP was installed without loopback support. Please re-install NPCAP";
        std::cerr << "Udpcap ERROR: " << human_readible_error_ << std::endl;
        RegCloseKey(hkey);
        return false;
      }

      std::wstring loopback_device_name_w;
      GetStringRegKey(hkey, L"LoopbackAdapter", loopback_device_name_w, L"");

      //if (loopback_device_name_w.empty())
      //{
      //  std::stringstream error_ss;

      //  error_ss << "Unable to retrieve NPCAP Loopback adapter name. Please reinstall Npcap:" << std::endl;
      //  error_ss << "    1) Uninstall Npcap" << std::endl;
      //  error_ss << "    2) Uninstall all \"Npcap Loopback Adapters\" from the device manager" << std::endl;
      //  error_ss << "    3) Uninstall all \"Microsoft KM_TEST Loopback Adapters\" from the device manager" << std::endl;
      //  error_ss << "    4) Install Npcap again";

      //  human_readible_error_ = error_ss.str();

      //  std::cerr << "Udpcap ERROR: " << human_readible_error_ << std::endl;

      //  RegCloseKey(hkey);
      //  return false;
      //}

      RegCloseKey(hkey);

      std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
      const std::string loopback_device_name = converter.to_bytes(loopback_device_name_w);

      // The Registry entry is in the form: \Device\{6DBF8591-55F9-4DEF-A317-54B9563A42E3}
      // We however only want the UUID:              6DBF8591-55F9-4DEF-A317-54B9563A42E3
      const size_t open_bracket_pos    = loopback_device_name.find('{');
      const size_t closing_bracket_pos = loopback_device_name.find('}', open_bracket_pos);
      loopback_device_uuid_string = loopback_device_name.substr(open_bracket_pos + 1, closing_bracket_pos - open_bracket_pos - 1);

      return true;
    }

    bool IsLoopbackDevice_NoLock(const std::string& device_name)
    {
      if (!loopback_device_name_initialized)
      {
        loopback_device_name_initialized = LoadLoopbackDeviceNameFromRegistry();
      }

      std::string lower_given_device_name    = device_name;
      std::string lower_loopback_device_name;

      if (!loopback_device_uuid_string.empty())
        lower_loopback_device_name = std::string("\\device\\{" + loopback_device_uuid_string + "}");
      else
        lower_loopback_device_name = "\\device\\npf_loopback";

      std::transform(lower_given_device_name.begin(), lower_given_device_name.end(), lower_given_device_name.begin(),
                    [](char c)
                    {return static_cast<char>(::tolower(c)); });

      std::transform(lower_loopback_device_name.begin(), lower_loopback_device_name.end(), lower_loopback_device_name.begin(),
                    [](char c)
                    {return static_cast<char>(::tolower(c)); });

      // At some point between NPCAP 0.9996 and NPCAP 1.10 the loopback device
      // was renamed to "\device\npf_loopback".
      // For newer NPCAP versions the complicated method to get the Device
      // UUID is obsolete. However, we leave the code in place, as it works
      // and still provides downwards compatibility to older NPCAP versions.
      return (lower_given_device_name == "\\device\\npf_loopback") || (lower_loopback_device_name == lower_given_device_name);
    }

    bool TestLoopbackDevice()
    {
      std::array<char, PCAP_ERRBUF_SIZE> errbuf{};
      pcap_if_t* alldevs_rawptr = nullptr;

      bool loopback_device_found = false;

      if (pcap_findalldevs(&alldevs_rawptr, errbuf.data()) == -1)
      {
        human_readible_error_ = "Error in pcap_findalldevs: " + std::string(errbuf.data());
        fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf.data());
        if (alldevs_rawptr != nullptr)
          pcap_freealldevs(alldevs_rawptr);
        return false;
      }

      // Check if the loopback device is accessible
      for (pcap_if_t* pcap_dev = alldevs_rawptr; pcap_dev != nullptr; pcap_dev = pcap_dev->next)
      {
        if (IsLoopbackDevice_NoLock(pcap_dev->name))
        {
          loopback_device_found = true;
        }
      }

      pcap_freealldevs(alldevs_rawptr);

      // if we didn't find the loopback device, the test has failed
      if (!loopback_device_found)
      {
        std::stringstream error_ss;

        error_ss << "Udpcap ERROR: Loopback adapter is inaccessible. On some systems the Npcap driver fails to start properly. Please open a command prompt with administrative privileges and run the following commands:" << std::endl;
        error_ss << "    When npcap was installed in normal mode:" << std::endl;
        error_ss << "       > sc stop npcap" << std::endl;
        error_ss << "       > sc start npcap" << std::endl;
        error_ss << "    When npcap was installed in WinPcap compatible mode:" << std::endl;
        error_ss << "       > sc stop npf" << std::endl;
        error_ss << "       > sc start npf";

        human_readible_error_ = error_ss.str();

        std::cerr << "Udpcap ERROR: " << human_readible_error_ << std::endl;

        return false;
      }

      return true;
    }
  }

  bool Initialize()
  {
    const std::lock_guard<std::mutex> npcap_lock(npcap_mutex);

    if (is_initialized) return true;

    human_readible_error_ = "Unknown error";

    std::cout << "Udpcap: Initializing Npcap..." << std::endl;

    LoadLoopbackDeviceNameFromRegistry();
    // Don't return false, as modern NPCAP will work without the registry key
    //if (!LoadLoopbackDeviceNameFromRegistry())
    //{
    //  return false;
    //}

    if (!loopback_device_uuid_string.empty())
      std::cout << "Udpcap: Using Loopback device " << loopback_device_uuid_string << std::endl;
    else
      std::cout << "Udpcap: Using Loopback device \\device\\npf_loopback" << std::endl;

    if (!LoadNpcapDlls())
    {
      std::cerr << "Udpcap ERROR: Unable to load Npcap. Please download and install Npcap from https://nmap.org/npcap/#download" << std::endl;
      return false;
    }

    if (!TestLoopbackDevice())
    {
      return false;
    }

    human_readible_error_ = "Npcap is ready";
    std::cout << "Udpcap: " << human_readible_error_ << std::endl;

    is_initialized = true;
    return true;
  }

  bool IsInitialized()
  {
    const std::lock_guard<std::mutex> npcap_lock(npcap_mutex);
    return is_initialized;
  }

  std::string GetLoopbackDeviceUuidString()
  {
    const std::lock_guard<std::mutex> npcap_lock(npcap_mutex);

    if (!loopback_device_name_initialized)
    {
      loopback_device_name_initialized = LoadLoopbackDeviceNameFromRegistry();
    }

    return loopback_device_uuid_string;
  }

  std::string GetLoopbackDeviceName()
  {
    const std::lock_guard<std::mutex> npcap_lock(npcap_mutex);

    if (!loopback_device_name_initialized)
    {
      LoadLoopbackDeviceNameFromRegistry();
      loopback_device_name_initialized = true;    // Even when we were not able to read the loopback device name, we assume it is present, as recent NPCAP versions don't create the specific adapter any more.
    }

    if (!loopback_device_uuid_string.empty())
      return "\\device\\npcap_{" + loopback_device_uuid_string + "}";
    else
      return "\\device\\npf_loopback";
  }

  bool IsLoopbackDevice(const std::string& device_name)
  {
    const std::lock_guard<std::mutex> npcap_lock(npcap_mutex);
    return IsLoopbackDevice_NoLock(device_name);
  }

  std::string GetHumanReadibleErrorText()
  {
    const std::lock_guard<std::mutex> npcap_lock(npcap_mutex);
    return human_readible_error_;
  }
}