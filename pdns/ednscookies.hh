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
#pragma once

#include "namespaces.hh"
#include "iputils.hh"

struct EDNSCookiesOpt
{
  string client;
  struct server {
    uint8_t version;
    uint32_t timestamp;
    string chash;
    string toString() const {
      string retval;
      retval.resize(8, 0);
      retval[0] = version;
      retval[4] = timestamp >> 24;
      retval[5] = timestamp >> 16;
      retval[6] = timestamp >> 8;
      retval[7] = timestamp;
      retval += chash;
      retval.resize(16,0);
      return retval;
    };
  } server;
};

bool getEDNSCookiesOptFromString(const string& option, EDNSCookiesOpt* eco);
bool createEDNSServerCookie(const string &secret, const ComboAddress &source, EDNSCookiesOpt &eco);
string makeEDNSCookiesOptString(const EDNSCookiesOpt& eco);
