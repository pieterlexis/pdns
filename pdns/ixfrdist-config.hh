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

#include "configparser.hh"

Config makeConfig();

/*
typedef struct {
  std::vector<ComboAddress> listen{"127.0.0.1:53", "[::1]:53"};
  std::vector<Netmask> acl{"127.0.0.0/8", "::1/128"};
  uint16_t axfrTimeout{20};
  uint16_t failedSOARetry{30};
  bool compress{false};
  uint16_t keep{20};
  uint16_t tcpInThreads{10};
  std::string workDir{"."};
  std::string uid;
  std::string gid;
  ComboAddress webserverListen{"127.0.0.1:8080"};
  std::vector<Netmask> webserverAcl{"127.0.0.0/8", "::1/128"};
  std::vector<DomainConfig> domains;
} IXFRDistConfig;

*/
