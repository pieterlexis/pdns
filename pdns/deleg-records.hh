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
#include <map>
#include <set>
#include "dnsname.hh"
#include "iputils.hh"
class DelegInfo
{
public:
  enum DelegInfoKey : uint16_t // NOLINT(performance-enum-size)
  {
    // draft-ietf-deleg-10
    /* When adding new values, you *must* update DelegInfo::DelegInfo(const std::string &key, const std::string &value)
     * in svc-record.cc with the new numbers
     */
    mandatory = 0,
    server_ipv4 = 1,
    server_ipv6 = 2,
    server_name = 3,
    include_delegparam = 4,
  };

  //! Returns the DelegInfoKey based on the input
  static DelegInfoKey keyFromString(const std::string& key);

  //! Returns the DelegInfoKey based on the input, generic is true when the format was 'keyNNNN'
  static DelegInfoKey keyFromString(const std::string& key, bool& generic);

  //! Returns the string value of the DelegInfoKey
  static std::string keyToString(const DelegInfoKey& key);

  //! empty Param, unusable
  DelegInfo() = delete;

  //! To create a multi-value DelegInfo with string values (like mandatory)
  DelegInfo(const DelegInfoKey& key, std::set<std::string>&& value);

  //! To create a multi-value DelegInfo with key values (like mandatory)
  DelegInfo(const DelegInfoKey& key, std::set<DelegInfoKey>&& value);

  //! To create a "generic" DelegInfo (for keyNNNNN)
  DelegInfo(const DelegInfoKey& key, const std::string& value);

  //! To create a server-ipv{4,6} DelegInfo
  DelegInfo(const DelegInfoKey& key, std::vector<ComboAddress>&& value, bool isAuto);

  //! To create a multi-value, DNSName DelegInfo (like sever-name and include-delegparam)
  DelegInfo(const DelegInfoKey& key, std::vector<DNSName>&& value, bool isAuto);

  bool operator<(const DelegInfo& other) const;

  bool operator==(const DelegInfo& other) const;

  bool operator!=(const DelegInfo& other) const;

  bool operator==(const DelegInfoKey& key) const
  {
    return key == d_key;
  }

  [[nodiscard]] DelegInfoKey getKey() const
  {
    return d_key;
  }

  [[nodiscard]] static std::set<DelegInfoKey> getAutoKeys()
  {
    return AutoKeys;
  }

  [[nodiscard]] const std::string& getValue() const;
  [[nodiscard]] const std::set<DelegInfoKey>& getMandatory() const;
  [[nodiscard]] const std::vector<DNSName>& getDnsNames() const;
  [[nodiscard]] const std::vector<ComboAddress>& getServerIPs() const;
  [[nodiscard]] bool canBeAuto() const;
  [[nodiscard]] const bool& isAuto() const;

private:
  DelegInfoKey d_key;
  std::string d_value; // For keyNNNNN vals

  std::set<DelegInfoKey> d_mandatory; // For mandatory
  std::vector<DNSName> d_dnsnames; // For server-name and include-delegparam
  std::vector<ComboAddress> d_serverips; // For server-ipv{6,4}

  bool d_doAuto{false}; // For server-{names,ipv{6,4}}, when true we need to do additional processing to get the addresseses from NS/A/AAAA records.

  static const std::map<std::string, DelegInfoKey> DelegInfos;
  static const std::set<DelegInfoKey> AutoKeys;
};
