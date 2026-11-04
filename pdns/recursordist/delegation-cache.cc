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

#include <limits>
#include <stdexcept>
#include <vector>

#include "delegation-cache.hh"
#include "dnsname.hh"
#include "qtype.hh"

extern __thread struct timeval g_now;

namespace rec
{
DelegationCacheEntry::DelegationCacheEntry(std::vector<DNSRecord>&& records) : d_records(std::move(records))
{
  d_type = DelegationType::UNKNOWN;
  uint32_t smallestTTL = std::numeric_limits<uint32_t>::max();

  for (const auto& record: d_records) {
    if (!QType::isDelegationType(record.d_type, true)) {
      continue;
    }

    smallestTTL = std::min(smallestTTL, record.d_ttl);

    if (record.d_type == QType::NS && d_type != DelegationType::DELEG) {
      d_type = DelegationType::NS;
    }
    if (QType::isDelegationType(record.d_type, false)) {
      d_type = DelegationType::DELEG;
    }
  }
  if (d_type == DelegationType::UNKNOWN) {
    throw std::invalid_argument("Could not determine delegation type");
  }

  // TODO: cap smallestTTL to a configured min and max.
  d_ttd = g_now.tv_sec + smallestTTL;
}

void DelegationCache::replace(const DNSName& name, std::vector<DNSRecord>&& records)
{
  DelegationCacheEntry entry(std::move(records));
  d_tree.add(name, std::move(entry));
}

const DelegationCacheEntry* DelegationCache::getBestFor(const DNSName& name) const
{
  DNSName lookupName = name;
  do {
    auto ret = d_tree.lookup(lookupName);
    if (ret->d_ttd > g_now.tv_sec) {
      return ret;
    }
    d_toPrune.insert(lookupName);
  } while (lookupName.chopOff());

  // TODO: throw? Reaching this means we had no root data in the cache
  return nullptr;
}

void DelegationCache::pruneExpired()
{

}

} // namespace rec
