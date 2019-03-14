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

#include "ixfrdist-config.hh"

Config makeConfig() {
  Config ret;

  ret.declareItem("listen", Config::ConfigItemType::IPEndpoint, "Listen addresses, ixfrdist will listen on bost TCP and UDP",
      R"(When no port is specified, 53 is used. When specifying ports for IPv6, use the
"bracket" notation:

    listen:
      - '127.0.0.1'
      - '::1'
      - '192.0.2.3:5300'
      - '[2001:DB8:1234::334]:5353'
      )",
      false,
      true,
      vector<ComboAddress>{"127.0.0.1:53", "[::1]:53"});
  ret.declareItem("acl", Config::ConfigItemType::IPNetmask, "Netmasks or IP addresses of hosts that are allowed to query ixfrdist.",
      R"(Hosts do not need a netmask:

   acl:
     - '127.0.0.0/8'
     - '::1'
     - '192.0.2.55'
     - '2001:DB8:ABCD::/48'
      )",
      false,
      true,
      vector<Netmask>{"127.0.0.0/8", "::1/128"});
  return ret;
}
