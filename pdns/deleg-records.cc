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
#include "deleg-records.hh"
#include "dnsname.hh"
#include <stdexcept>
#include <vector>

const std::map<std::string, DelegInfo::DelegInfoKey> DelegInfo::DelegInfos = {
  {"mandatory", DelegInfo::DelegInfoKey::mandatory},
  {"server-ipv4", DelegInfo::DelegInfoKey::server_ipv4},
  {"server-ipv6", DelegInfo::DelegInfoKey::server_ipv6},
  {"server-name", DelegInfo::DelegInfoKey::server_name},
  {"include-delegparam", DelegInfo::DelegInfoKey::include_delegparam},
};

DelegInfo::DelegInfoKey DelegInfo::keyFromString(const std::string& key)
{
  bool ignored{false};
  return keyFromString(key, ignored);
}

DelegInfo::DelegInfoKey DelegInfo::keyFromString(const std::string& key, bool& generic)
{
  auto it = DelegInfos.find(key);
  if (it != DelegInfos.cend()) {
    generic = false;
    return it->second;
  }
  if (key.substr(0, 3) == "key") {
    try {
      generic = true;
      return DelegInfo::DelegInfoKey(pdns::checked_stoi<uint16_t>(key.substr(3)));
    }
    catch (...) {
    }
  }
  throw std::invalid_argument("DelegInfoKey '" + key + "' is not recognized or in keyNNNN format");
}

std::string DelegInfo::keyToString(const DelegInfoKey& key)
{
  auto ret = std::find_if(DelegInfos.begin(), DelegInfos.end(), [&](const std::pair<std::string, DelegInfo::DelegInfoKey>& e) { return e.second == key; });
  if (ret != DelegInfos.end()) {
    return ret->first;
  }
  return "key" + std::to_string(key);
}

DelegInfo::DelegInfo(const DelegInfoKey& key, std::set<std::string>&& value) :
  d_key(key)
{
  if (d_key != DelegInfoKey::mandatory) {
    throw std::invalid_argument("can not create DelegInfo for " + keyToString(key) + " with a string-set value");
  }
  if (d_key == DelegInfoKey::mandatory) {
    for (auto const& val : value) {
      d_mandatory.insert(keyFromString(val));
    }
  }
}

DelegInfo::DelegInfo(const DelegInfoKey& key, std::set<DelegInfoKey>&& value) :
  d_key(key), d_mandatory(std::move(value))
{
  if (d_key != DelegInfoKey::mandatory) {
    throw std::invalid_argument("can not create DelegInfo for " + keyToString(key) + " with a DelegInfoKey-set value");
  }
}

DelegInfo::DelegInfo(const DelegInfoKey& key, const std::string& value) :
  d_key(key)
{
  if (d_key < 5) {
    throw std::invalid_argument("can not create SvcParam for " + keyToString(key) + " with a string value");
  }
  d_value = value;
}

DelegInfo::DelegInfo(const DelegInfoKey& key, std::vector<ComboAddress>&& value) :
  d_key(key), d_serverips(std::move(value))
{
  if (d_key != DelegInfoKey::server_ipv4 && d_key != DelegInfoKey::server_ipv6) {
    throw std::invalid_argument("can not create DelegInfo for " + keyToString(key) + "with ComboAddress values");
  }
  for (auto const& addr : d_serverips) {
    if (d_key == DelegInfoKey::server_ipv6 && !addr.isIPv6()) {
      throw std::invalid_argument("non-IPv6 address ('" + addr.toString() + "') passed for " + keyToString(key));
    }
    if (d_key == DelegInfoKey::server_ipv4 && !addr.isIPv4()) {
      throw std::invalid_argument("non-IPv4 address ('" + addr.toString() + "') passed for " + keyToString(key));
    }
  }
}

DelegInfo::DelegInfo(const DelegInfoKey& key, std::vector<DNSName>&& value) :
  d_key(key), d_dnsnames(std::move(value))
{
  if (d_key != DelegInfoKey::server_name && d_key != DelegInfoKey::include_delegparam) {
    throw std::invalid_argument("can not create DelegInfo for " + keyToString(key) + "with ComboAddress values");
  }
}

//! This ensures an std::set<DelegInfo> will be sorted by key
bool DelegInfo::operator<(const DelegInfo& other) const
{
  return this->getKey() < other.getKey();
}

bool DelegInfo::operator==(const DelegInfo& other) const
{
  if (this->getKey() != other.getKey()) {
    return false;
  }
  switch (this->d_key) {
  case DelegInfoKey::mandatory:
    return this->getMandatory() == other.getMandatory();
  case DelegInfoKey::server_ipv4: /* fallthrough */
  case DelegInfoKey::server_ipv6:
    return (this->getServerIPs() == other.getServerIPs());
  case DelegInfoKey::server_name: /* fallthrough */
  case DelegInfoKey::include_delegparam:
    return (this->getDnsNames() == other.getDnsNames());
  default:
    return this->getValue() == other.getValue();
  }
}

bool DelegInfo::operator!=(const DelegInfo& other) const
{
  return !(*this == other);
}

const std::string& DelegInfo::getValue() const
{
  if (d_key < 5) {
    throw std::invalid_argument("getValue called for non-single value key '" + keyToString(d_key) + "'");
  }
  return d_value;
}

const std::set<DelegInfo::DelegInfoKey>& DelegInfo::getMandatory() const
{
  if (d_key != DelegInfo::mandatory) {
    throw std::invalid_argument("getMandatory called for non-mandatory key '" + keyToString(d_key) + "'");
  }
  return d_mandatory;
}

const std::vector<ComboAddress>& DelegInfo::getServerIPs() const
{
  if (d_key != DelegInfoKey::server_ipv4 && d_key != DelegInfoKey::server_ipv6) {
    throw std::invalid_argument("getServerIPs called for non-IP address key '" + keyToString(d_key) + "'");
  }
  return d_serverips;
}

const std::vector<DNSName>& DelegInfo::getDnsNames() const
{
  if (d_key != DelegInfoKey::server_name && d_key != DelegInfoKey::include_delegparam) {
    throw std::invalid_argument("getDnsNames called for non-DNSName key '" + keyToString(d_key) + "'");
  }
  return d_dnsnames;
}
