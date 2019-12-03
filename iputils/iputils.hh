/*
 * This file is part of PowerDNS or weakforced.
 * Copyright -- PowerDNS.COM B.V. and its contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * In addition, for the avoidance of any doubt, permission is granted to
 * link this program with OpenSSL and to (re)distribute the binaries
 * produced as the result of such linking.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#pragma once

#include <arpa/inet.h>
#include <cstring> // for memcmp
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <tuple>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#endif

// for illumos
#ifdef BE_64

#define htobe16(x) BE_16(x)
#define htole16(x) LE_16(x)
#define be16toh(x) BE_IN16(&(x))
#define le16toh(x) LE_IN16(&(x))

#define htobe32(x) BE_32(x)
#define htole32(x) LE_32(x)
#define be32toh(x) BE_IN32(&(x))
#define le32toh(x) LE_IN32(&(x))

#define htobe64(x) BE_64(x)
#define htole64(x) LE_64(x)
#define be64toh(x) BE_IN64(&(x))
#define le64toh(x) LE_IN64(&(x))

#endif

#ifdef __FreeBSD__
#include <sys/endian.h>
#endif

namespace iputils
{
union ComboAddress
{
public:
  /**
     * @brief Are two ComboAddresses equal
     * 
     * Checks both the address and the port
     * 
     * @param rhs 
     * @return true   They were equal
     * @return false  They were not equal
     */
  bool operator==(const ComboAddress& rhs) const {
    if (std::tie(sin4.sin_family, sin4.sin_port) != std::tie(rhs.sin4.sin_family, rhs.sin4.sin_port))
      return false;
    if (sin4.sin_family == AF_INET)
      return sin4.sin_addr.s_addr == rhs.sin4.sin_addr.s_addr;
    else
      return memcmp(&sin6.sin6_addr.s6_addr, &rhs.sin6.sin6_addr.s6_addr, sizeof(sin6.sin6_addr.s6_addr)) == 0;
  }

  /**
     * @brief Are two ComboAddresses not equal
     * 
     * @param rhs 
     * @return true   They were not equal
     * @return false  They were equal
     */
  bool operator!=(const ComboAddress& rhs) const {
    return (!operator==(rhs));
  }

  /**
     * @brief Is the ComboAddress smaller than the rhs?
     * 
     * Compares in this order:
     * 
     * IPv6 > IPv4
     * Port number
     * IP address 
     * 
     * so:
     * 
     * [::1]:8000 > [2001:DB8::12]:2000
     * [::1]:1 > 192.0.2.1:3000
     * 192.0.2.1:4000 > 192.0.2.1:3000
     * 2001:DB8::12 > 2001:DB8::11 
     * 
     * @param rhs 
     * @return true   rhs was larger
     * @return false  we were larger
     */
  bool operator<(const ComboAddress& rhs) const {
    if (sin4.sin_family == 0) {
      return false;
    }
    if (std::tie(sin4.sin_family, sin4.sin_port) < std::tie(rhs.sin4.sin_family, rhs.sin4.sin_port))
      return true;
    if (std::tie(sin4.sin_family, sin4.sin_port) > std::tie(rhs.sin4.sin_family, rhs.sin4.sin_port))
      return false;

    if (sin4.sin_family == AF_INET)
      return sin4.sin_addr.s_addr < rhs.sin4.sin_addr.s_addr;
    else
      return memcmp(&sin6.sin6_addr.s6_addr, &rhs.sin6.sin6_addr.s6_addr, sizeof(sin6.sin6_addr.s6_addr)) < 0;
  }

  /**
     * @brief Are we larger than the rhs?
     * 
     * @see operator<
     * 
     * @param rhs 
     * @return true   We were larger
     * @return false  rhs was larger
     */
  bool operator>(const ComboAddress& rhs) const {
    return rhs.operator<(*this);
  }

  /**
     * @brief Get the socklen_t
     * 
     * @return socklen_t 
     */
  socklen_t getSocklen() const {
    if (sin4.sin_family == AF_INET)
      return sizeof(sin4);
    else
      return sizeof(sin6);
  }

  /**
    * @brief Construct a new ComboAddress
    * 
    * It is empty
    */
  ComboAddress() {
    sin4.sin_family = AF_INET;
    sin4.sin_addr.s_addr = 0;
    sin4.sin_port = 0;
  }

  /**
     * @brief Construct a new ComboAddress from a sockaddr
     * 
     * @param sa     struct sockaddr* to create the ComboAddress from
     * @param salen  size of sa
     * @throw std::logic_error  when salen > sizeof(struct sockaddr_in6)
     */
  ComboAddress(const struct sockaddr* sa, socklen_t salen) {
    setSockaddr(sa, salen);
  };

  /**
     * @brief Construct a new ComboAddress from a sockaddr_in6
     * 
     * @param sa   struct sockaddr_in6* to create ComboAddress from
     */
  ComboAddress(const struct sockaddr_in6* sa) {
    setSockaddr((const struct sockaddr*)sa, sizeof(struct sockaddr_in6));
  };

  /**
     * @brief Construct a new ComboAddress from a sockaddr_in
     * 
     * @param sa   struct sockaddr_in* to create ComboAddress from
     */
  ComboAddress(const struct sockaddr_in* sa) {
    setSockaddr((const struct sockaddr*)sa, sizeof(struct sockaddr_in));
  };

  /**
   * @brief Construct a new ComboAddress object
   * 
   * @param str   String to convert
   * @param port  Port to set if str did not contain one
   */
  explicit ComboAddress(const std::string& str, uint16_t port = 0) {
    memset(&sin6, 0, sizeof(sin6));
    sin4.sin_family = AF_INET;
    sin4.sin_port = 0;
    try {
      makeIPv4sockaddr(str, &sin4);
    }
    catch (...) {
      sin6.sin6_family = AF_INET6;
      try {
        makeIPv6sockaddr(str, &sin6);
      }
      catch (...) {
        throw std::logic_error("Unable to convert presentation address '" + str + "'");
      }
    }
    if (!sin4.sin_port) // 'str' overrides port!
      sin4.sin_port = htons(port);
  }

  /**
     * @brief Are we IPv6?
     * 
     * @return true 
     * @return false 
     */
  bool isIpv6() const {
    return sin6.sin6_family == AF_INET6;
  }

  /**
     * @brief Are we IPv4?
     * 
     * @return true 
     * @return false 
     */
  bool isIpv4() const {
    return sin4.sin_family == AF_INET;
  }

  /**
     * @brief Are we an IPv4 address mapped in IPv6
     * 
     * e.g. are we ::192.0.2.1
     * 
     * @return true 
     * @return false 
     */
  bool isMappedIPv4() const {
    if (sin4.sin_family != AF_INET6)
      return false;

    int n = 0;
    const unsigned char* ptr = (unsigned char*)&sin6.sin6_addr.s6_addr;
    for (n = 0; n < 10; ++n)
      if (ptr[n])
        return false;

    for (; n < 12; ++n)
      if (ptr[n] != 0xff)
        return false;

    return true;
  }

  /**
     * @brief Returns an IPv4 ComboAddress from one mapped in IPv6
     * 
     * @return ComboAddress 
     * @throw std::logic_error when we are are not a mapped v4 address
     */
  ComboAddress mapToIPv4() const {
    if (!isMappedIPv4())
      throw std::logic_error("ComboAddress can't map non-mapped IPv6 address back to IPv4");
    ComboAddress ret;
    ret.sin4.sin_family = AF_INET;
    ret.sin4.sin_port = sin4.sin_port;

    const unsigned char* ptr = (unsigned char*)&sin6.sin6_addr.s6_addr;
    ptr += (sizeof(sin6.sin6_addr.s6_addr) - sizeof(ret.sin4.sin_addr.s_addr));
    memcpy(&ret.sin4.sin_addr.s_addr, ptr, sizeof(ret.sin4.sin_addr.s_addr));
    return ret;
  }

  /**
     * @brief Get the IP Address without port in presentation format
     * 
     * @return std::string 
     * @see toStringWithPort
     */
  std::string toString() const {
    char host[1024];
    if (sin4.sin_family && !getnameinfo((struct sockaddr*)this, getSocklen(), host, sizeof(host), 0, 0, NI_NUMERICHOST))
      return host;
    else
      return "invalid";
  }

  /**
     * @brief Get the IP Address with port in presentation format
     * 
     * @return std::string 
     */
  std::string toStringWithPort() const {
    if (sin4.sin_family == AF_INET)
      return toString() + ":" + std::to_string(ntohs(sin4.sin_port));
    else
      return "[" + toString() + "]:" + std::to_string(ntohs(sin4.sin_port));
  }

  /**
     * @brief Truncate the address to N bits
     * 
     * Zeroes out the right-most bits until only `bits` have value
     * 
     * @param bits 
     * @throw std::logic_error when bits >= bits in address
     */
  void truncate(unsigned int bits) {
    uint8_t* start;
    int len = 4;
    if (sin4.sin_family == AF_INET) {
      if (bits >= 32)
        throw std::logic_error("Can't truncate to " + std::to_string(bits) + " bits for IPv4");
      start = (uint8_t*)&sin4.sin_addr.s_addr;
      len = 4;
    }
    else {
      if (bits >= 128)
        throw std::logic_error("Can't truncate to " + std::to_string(bits) + " bits for IPv6");
      start = (uint8_t*)&sin6.sin6_addr.s6_addr;
      len = 16;
    }

    auto tozero = len * 8 - bits; // if set to 22, this will clear 1 byte, as it should
    memset(start + len - tozero / 8, 0, tozero / 8); // blot out the whole bytes on the right
    auto bitsleft = tozero % 8; // 2 bits left to clear

    // a b c d, to truncate to 22 bits, we just zeroed 'd' and need to zero 2 bits from c
    // so and by '11111100', which is ~((1<<2)-1)  = ~3
    uint8_t* place = start + len - 1 - tozero / 8;
    *place &= (~((1 << bitsleft) - 1));
  }

  /**
     * @brief Clear this ComboAddress
     * 
     * Zeroes out the adress and port
     */
  void reset() {
    memset(&sin6, 0, sizeof(sin6));
  }

  /**
     * @brief Set the Port
     * 
     * @param p 
     */
  void setPort(uint16_t p) {
    sin4.sin_port = htons(p);
  }

  /**
   * @brief Get the port of this ComboAddress
   * 
   * @return uint16_t 
   */
  uint16_t getPort() const {
    return ntohs(sin4.sin_port);
  }

  /**
   * @brief Is this ComboAddress empty?
   * 
   * @return true 
   * @return false 
   */
  bool empty() {
    return sin4.sin_family == 0;
  }

protected:
  struct sockaddr_in sin4;
  struct sockaddr_in6 sin6;

  void setSockaddr(const struct sockaddr* sa, socklen_t salen) {
    if (salen > sizeof(struct sockaddr_in6)) {
      std::logic_error("ComboAddress can't handle other than sockaddr_in or sockaddr_in6");
    }
    memcpy(this, sa, salen);
  }

  /**
 * @brief Converts a string to an unsigned integer, throwing when needed
 * 
 * @param str as std::stoul
 * @param idx  as std::stoul
 * @param base  as std::stoul
 * @return unsigned int
 */
  unsigned int pdns_stou(const std::string& str, size_t* idx = nullptr, int base = 10) {
    if (str.empty())
      return 0; // compability
    unsigned long result = std::stoul(str, idx, base);
    if (result > std::numeric_limits<unsigned int>::max()) {
      throw std::out_of_range("stou");
    }
    return static_cast<unsigned int>(result);
  }

  /**
 * @brief Splits string into 2 parts by a separator
 * 
 * If there is no separator, or it is a the end, the whole inp string
 * is returned in the first element of the pair
 * 
 * @param inp   String to split
 * @param sepa  Separator
 * @return std::pair<std::string, std::string>  These are the split fields
 */
  std::pair<std::string, std::string> splitField(const std::string& inp, char sepa) {
    std::pair<std::string, std::string> ret;
    std::string::size_type cpos = inp.find(sepa);
    if (cpos == std::string::npos)
      ret.first = inp;
    else {
      ret.first = inp.substr(0, cpos);
      ret.second = inp.substr(cpos + 1);
    }
    return ret;
  }

  /**
 * @brief Takes an IPv6 address string (with or without port) and 
 * 
 * @param addr 
 * @param ret 
 * @return int 
 */
  void makeIPv6sockaddr(const std::string& addr, struct sockaddr_in6* ret) {
    if (addr.empty())
      throw std::runtime_error("Address cannot be empty");
    std::string ourAddr(addr);
    int port = -1;
    if (addr[0] == '[') { // [::]:53 style address
      std::string::size_type pos = addr.find(']');
      if (pos == std::string::npos || pos + 2 > addr.size() || addr[pos + 1] != ':')
        throw std::out_of_range("Invalid braced IPv6 address");
      ourAddr.assign(addr.c_str() + 1, pos - 1);
      port = atoi(addr.c_str() + pos + 2);
    }
    ret->sin6_scope_id = 0;
    ret->sin6_family = AF_INET6;

    if (inet_pton(AF_INET6, ourAddr.c_str(), (void*)&ret->sin6_addr) != 1) {
      struct addrinfo* res;
      struct addrinfo hints;
      memset(&hints, 0, sizeof(hints));

      hints.ai_family = AF_INET6;
      hints.ai_flags = AI_NUMERICHOST;

      int error;
      if ((error = getaddrinfo(ourAddr.c_str(), 0, &hints, &res))) { // this is correct
        throw std::runtime_error(gai_strerror(error));
      }

      memcpy(ret, res->ai_addr, res->ai_addrlen);
      freeaddrinfo(res);
    }

    if (port >= 0)
      ret->sin6_port = htons(port);
  }

  void makeIPv4sockaddr(const std::string& str, struct sockaddr_in* ret) {
    if (str.empty()) {
      throw std::runtime_error("Address cannot be empty");
    }
    struct in_addr inp;

    std::string::size_type pos = str.find(':');
    if (pos == std::string::npos) { // no port specified, not touching the port
      int error;
      if ((error = inet_aton(str.c_str(), &inp))) {
        ret->sin_addr.s_addr = inp.s_addr;
        return;
      }
      throw std::runtime_error("Invalid address");
    }
    if (!*(str.c_str() + pos + 1)) // trailing :
      throw std::runtime_error("Trailing ':' in address");

    char* eptr = (char*)str.c_str() + str.size();
    int port = strtol(str.c_str() + pos + 1, &eptr, 10);
    if (*eptr)
      throw std::runtime_error("Invalid port");

    int error;
    ret->sin_port = htons(port);
    if ((error = inet_aton(str.substr(0, pos).c_str(), &inp))) {
      ret->sin_addr.s_addr = inp.s_addr;
      return;
    }
    throw std::runtime_error("Invalid address");
  }
};
} // namespace iputils