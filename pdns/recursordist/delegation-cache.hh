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

#include <vector>

#include "dnsname.hh"
#include "dnsparser.hh"

namespace rec
{

enum DelegationType {
  UNKNOWN,  // The default, should never occur in the cache
  NS,       // NS records are used for this delegation
  DELEG,    // DELEG (or perhaps even DelExt) records are used for this delegation
  NXD,      // We have an AA=1 NXDomain for this name
};

class DelegationCacheEntry {
public:
  // XXX: Do we need this?
  // DelegationCacheEntry(std::vector<DNSRecord>&& records, DelegationType type, time_t ttd) : d_records(std::move(records)), d_type(type), d_ttd(ttd) {};
  DelegationCacheEntry(std::vector<DNSRecord>&& records);
  ~DelegationCacheEntry() = default;
  DelegationCacheEntry(const DelegationCacheEntry&) = delete;
  DelegationCacheEntry(DelegationCacheEntry&&) = delete;
  DelegationCacheEntry& operator=(const DelegationCacheEntry&) = delete;

  // TODO: DNSSEC records?
  std::vector<DNSRecord> d_records;

  // TODO: DNSSEC validation state?
  // TODO: AuthorityRecs
  // TODO: RRSIGs + NSEC
  DelegationType d_type;
  time_t d_ttd;
};

class DelegationCache {
public:
  DelegationCache() = default;
  ~DelegationCache() = default;
  DelegationCache(const DelegationCache&) = delete;
  DelegationCache(DelegationCache&&) = delete;
  DelegationCache& operator=(const DelegationCache&) = delete;

  // void replace(const DNSName& name, DelegationCacheEntry&& entry);
  void replace(const DNSName& name, std::vector<DNSRecord>&& records);
  const DelegationCacheEntry* getBestFor(const DNSName& name) const;

  // TODO: serve stale?
  void pruneExpired();

private:
  SuffixMatchTree<DelegationCacheEntry> d_tree;
  mutable std::set<DNSName> d_toPrune;
};
} // namespace rec
