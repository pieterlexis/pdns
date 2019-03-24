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
#include "ednscookies.hh"
#include <arpa/inet.h>
#include "arguments.hh"
#include "siphash.c"

bool getEDNSCookiesOptFromString(const string& option, EDNSCookiesOpt* eco)
{
  return getEDNSCookiesOptFromString(option.c_str(), option.length(), eco);
}

bool getEDNSCookiesOptFromString(const char* option, unsigned int len, EDNSCookiesOpt* eco)
{
  if(len != 8 && len < 16)
    return false;
  eco->client = string(option, 8);
  if (len > 8) {
    eco->server = string(option + 8, len - 8);
  }
  return true;
}

string makeEDNSCookiesOptString(const EDNSCookiesOpt& eco)
{
  string ret;
  if (eco.client.length() != 8)
    return ret;
  if (eco.server.length() != 0 && (eco.server.length() < 8 || eco.server.length() > 32))
    return ret;
  ret.assign(eco.client);
  if (eco.server.length() != 0)
    ret.append(eco.server);
  return ret;
}

bool createEDNSServerCookie(const string &secret, EDNSCookiesOpt &eco) {
  if (eco.client.size() != 8) {
    return false;
  }

  string cookie = makeEDNSCookiesOptString(eco);

  cookie.resize(8+16); // Client cookie (8) + server cookie (16)
  cookie[8] = 1; // version
  cookie[9] = 4; // Algorithm (siphash)
  cookie[10] = 0; // Reserved
  cookie[11] = 0; // Reserved

  // Add the timestamp
  uint32_t now = static_cast<uint32_t>(time(nullptr));
  cookie[12] = now >> 24;
  cookie[13] = now >> 16;
  cookie[14] = now >> 8;
  cookie[15] = now;

  // Add the hash (in, inlen, secret, out, outlen)
  siphash(reinterpret_cast<const uint8_t*>(&cookie[0]), 16,
      reinterpret_cast<const uint8_t*>(&secret[0]),
      reinterpret_cast<uint8_t*>(&cookie[16]), 8);

  eco.server = cookie.substr(8);
  return true;
}
