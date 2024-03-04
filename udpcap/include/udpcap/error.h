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

#pragma once

#include <string>

namespace Udpcap
{
  class Error
  {
  //////////////////////////////////////////
  // Data model
  //////////////////////////////////////////
  public:
    enum ErrorCode
    {
      // Generic
      OK,
      GENERIC_ERROR,

      // NPCAP errors
      NPCAP_NOT_INITIALIZED,

      // Socket errors
      NOT_BOUND,
      TIMEOUT,
      SOCKET_CLOSED,
    };

  //////////////////////////////////////////
  // Constructor & Destructor
  //////////////////////////////////////////
  public:
    Error(ErrorCode error_code, const std::string& message) : error_code_(error_code), message_(message) {}
    Error(ErrorCode error_code) : error_code_(error_code) {}

    // Copy constructor & assignment operator
    Error(const Error& other)            = default;
    Error& operator=(const Error& other) = default;

    // Move constructor & assignment operator
    Error(Error&& other)                 = default;
    Error& operator=(Error&& other)      = default;

    ~Error() = default;

  //////////////////////////////////////////
  // Public API
  //////////////////////////////////////////
  public:
    inline std::string GetDescription() const
    {
      switch (error_code_)
      {
      // Generic
      case OK:                                    return "OK";                                            break;
      case GENERIC_ERROR:                         return "Error";                                         break;

      case NPCAP_NOT_INITIALIZED:                 return "Npcap not initialized";                         break;

      case NOT_BOUND:                             return "Socket not bound";                              break;
      case TIMEOUT:                               return "Timeout";                                       break;
      case SOCKET_CLOSED:                         return "Socket closed";                                 break;

      default:                                    return "Unknown error";
      }
    }

    inline std::string ToString() const
    {
      return (message_.empty() ? GetDescription() : GetDescription() + " (" + message_ + ")");
    }

    const inline std::string& GetMessage() const
    {
      return message_;
    }

    inline ErrorCode GetErrorCode() const
    {
      return error_code_;
    }

  //////////////////////////////////////////
  // Operators
  //////////////////////////////////////////
    inline operator bool() const { return error_code_ != ErrorCode::OK; }
    inline bool operator== (const Error& other)    const { return error_code_ == other.error_code_; }
    inline bool operator== (const ErrorCode other) const { return error_code_ == other; }
    inline bool operator!= (const Error& other)    const { return error_code_ != other.error_code_; }
    inline bool operator!= (const ErrorCode other) const { return error_code_ != other; }

    inline Error& operator=(ErrorCode error_code)
    {
      error_code_ = error_code;
      return *this;
    }

  //////////////////////////////////////////
  // Member Variables
  //////////////////////////////////////////
  private:
    ErrorCode   error_code_;
    std::string message_;
  };

} // namespace Udpcap
