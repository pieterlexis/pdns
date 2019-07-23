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
#include "siphash.c"
#include "iputils.hh"
#include "misc.hh"

bool getEDNSCookiesOptFromString(const string& option, EDNSCookiesOpt* eco)
{
  auto len = option.size();
  if(len != 8 && len != 24) {
    return false;
  }
  eco->client = option.substr(0, 8);
  eco->server.version = 0;
  if (len == 24) {
    eco->server.version = static_cast<uint8_t>(option[8]);
    eco->server.reserved[0] = static_cast<uint8_t>(option[9]);
    eco->server.reserved[1] = static_cast<uint8_t>(option[10]);
    eco->server.reserved[2] = static_cast<uint8_t>(option[11]);
    eco->server.timestamp = (option[12] << 24) +
                            (option[13] << 16) +
                            (option[14] << 8) +
                            option[15];
    eco->server.chash = option.substr(16, 8);
  }
  return true;
}

string makeEDNSCookiesOptString(const EDNSCookiesOpt& eco)
{
  string ret;
  if (eco.client.length() != 8)
    return ret;
  ret.assign(eco.client);
  if (eco.server.version != 0)
    ret.append(eco.server.toString());
  return ret;
}

void makeSipHash(const string &secret, const string &toHash, string &theHash) {
  theHash.clear();
  theHash.resize(8);
  siphash(reinterpret_cast<const uint8_t*>(&toHash[0]), toHash.size(), // in, inlen,
      reinterpret_cast<const uint8_t*>(&secret[0]),                    // secret,
      reinterpret_cast<uint8_t*>(&theHash[0]), 8);                     // out, outlen
}

bool createEDNSServerCookie(const string &secret, const ComboAddress &source, EDNSCookiesOpt &eco) {
  eco.server.version = 1;
  eco.server.reserved[0] = 0;
  eco.server.reserved[1] = 0;
  eco.server.reserved[2] = 0;
  eco.server.timestamp = static_cast<uint32_t>(time(nullptr));

  string toHash = makeEDNSCookiesOptString(eco);
  toHash.resize(16); // Remove the existing hash, if any
  toHash += source.toByteString();
  string h;

  makeSipHash(secret, toHash, h);
  eco.server.chash = h;
  return true;
}

bool checkEDNSCookie(const string &secret, const ComboAddress &source, const EDNSCookiesOpt &eco) {
  if (eco.client.size() != 8) {
    return false;
  }

  if (eco.server.chash.size() != 8) {
    return false;
  }

  if (eco.server.version != 1) {
    return false;
  }

  for (auto &r : eco.server.reserved) {
    if (r != 0) {
      return false;
    }
  }

  uint32_t now = static_cast<uint32_t>(time(nullptr));
  if (eco.server.timestamp - 300 > now) {
    return false;
  }

  if (eco.server.timestamp + 3600 < now) {
    return false;
  }

  string toHash = makeEDNSCookiesOptString(eco);
  toHash.resize(16); // Remove the existing hash
  toHash += source.toByteString();

  string h;
  makeSipHash(secret, toHash, h);
  return h == eco.server.chash;
}
