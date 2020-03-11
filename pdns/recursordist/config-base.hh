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

#include <string>
#include <vector>
#include "iputils.hh"
#include "dnsname.hh"
#include "dnsrecords.hh"

namespace pdns
{
namespace config
{
  struct dnssec_config
  {
    std::string validation;
    bool log_bogus;
    std::vector<std::pair<DNSName, std::shared_ptr<DSRecordContent>>> trust_anchors;
    std::vector<std::pair<DNSName, std::string>> negative_trust_anchors;
  };

  struct listen_address
  {
    ComboAddress address;
    bool non_local_bind;
  };

  class RecursorConfig
  {
    // Base class for all different config parsers
  public:
    virtual dnssec_config dnssecConfig() = 0;
    virtual std::vector<struct listen_address> listenAddresses() = 0;
  };
}
}
