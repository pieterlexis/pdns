/*
 * This file is part of PowerDNS or dnsdist.
 * Copyright -- PowerDNS.COM B.V. and its contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
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

#include <string>
#include <stdexcept>
#include "iputils.hh"
#include "dnsdist-cookies.hh"
#include "siphash.c"

/** \brief Takes a cookie from a client and returns the full client+server cookie
 *
 * \param[in] clientCookie The bytes from the client's cookie, can be either only the client cookie or a full (client+server) cookie
 * \param[in] clientAddr Used during hashing
 * \param[in] theTime When not a nullptr, this time is used when calculating the hash. When not provided, the current time is used
 * \return a string that is the full client+server cookie
 */
std::string DNSDistCookieContext::makeCookie(const std::string &clientCookie, const ComboAddress &clientAddr, const time_t *theTime) {
  std::string cookie;
  if (clientCookie.size() != 8 && clientCookie.size() != 24) {
    throw std::range_error("Cookie has incorrect size " + std::to_string(clientCookie.size()));
  }
  cookie = clientCookie;

  // Clear the cookie to return, except the client-cookie
  cookie.resize(8, 0);
  cookie.resize(16, 0); // cookie is now the size of clientcookie + version + reserved + timestamp

  cookie[8] = 1; // Version

  // Add the timestamp
  uint32_t now;
  if (theTime != nullptr) {
    now = static_cast<uint32_t>(*theTime);
  } else {
    now = static_cast<uint32_t>(time(nullptr));
  }

  // TODO use string::assign or string +=
  cookie[12] = now >> 24;
  cookie[13] = now >> 16;
  cookie[14] = now >> 8;
  cookie[15] = now;

  std::string toHash = cookie;
  cookie.resize(24); // final size
  toHash += clientAddr.toByteString();

  // Hash toHash and plonk the result into the last 8 bytes of cookie
  // (in, inlen, secret, out, outlen)
  siphash(reinterpret_cast<const uint8_t*>(&toHash[0]), toHash.size(),
      reinterpret_cast<const uint8_t*>(&d_secret[0]),
      reinterpret_cast<uint8_t*>(&cookie[16]), 8);
  return cookie;
}

/** \brief Checks if clientCookie from clientAddr is correct
 *
 * \param[in] clientCookie The bytes from the client's cookie
 * \param[in] clientAddr The client's address
 * \return whether or not the cookie was valid
 */
bool DNSDistCookieContext::isCorrectCookie(const std::string &clientCookie, const ComboAddress &clientAddr) {
  if (clientCookie.size() == 8) {
    // Can't check something without a servercookie
    return false;
  }

  if (clientCookie.size() != 24) {
    throw std::range_error("Client Cookie has wrong size " + std::to_string(clientCookie.size()));
  }

  std::string toHashCookie = clientCookie.substr(0, 16);
  toHashCookie += clientAddr.toByteString();

  std::string toCheckVal;
  toCheckVal.resize(8);

  siphash(reinterpret_cast<const uint8_t*>(&toHashCookie[0]), toHashCookie.size(),
      reinterpret_cast<const uint8_t*>(&d_secret[0]),
      reinterpret_cast<uint8_t*>(&toCheckVal[0]), 8);

  return clientCookie.compare(16, 8, toCheckVal) == 0;
}
