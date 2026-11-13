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

class DelegationInfo {
public:

  enum DelegationType {
    UNKNOWN,          // The default, should never occur in the cache
    LOCALAUTH,        // We have a locally hosted auth zone
    FORWARD,          // We have a forward configured
    FORWARDNORECURSE, // We have a forward configured that requires no RD
    NS,               // NS records are used for this delegation
    SERVERNAMEDELEG,  // DELEG with ServerName (so basically NS)
    ADDRESSDELEG,     // DELEG with server-ipv{4,6}
    PARAMDELEG,       // DELEG with include-delegparam
    NXD,              // We have an AA=1 NXDomain for this name
  };

  DelegationInfo();
  DelegationInfo(std::vector<DNSRecord>&& records);

  ~DelegationInfo() = default;
  DelegationInfo(const DelegationInfo&) = delete;
  DelegationInfo(DelegationInfo&&) = default;
  DelegationInfo& operator=(const DelegationInfo&) = delete;
  void clear()
  {
    d_type = DelegationType::UNKNOWN;
    d_names.clear();
    d_addrs.clear();
  }

  DelegationType d_type;
  vector<DNSName> d_names; // Names for NS servers or DELEG server-name or DELEG include-delegparam
  vector<ComboAddress> d_addrs; // for forwards, NOT for NS or DELEG

  // std::vector<DNSRecord> d_records;
  // TODO: RRSIGs + NSEC
  // TODO: DNSSEC validation state?
  // TODO: AuthorityRecs
};

class DelegationCacheEntry {
public:
  DelegationCacheEntry() = delete;
  DelegationCacheEntry(DelegationInfo&& info, time_t ttd) : info(std::move(info)), ttd(ttd) {}
  ~DelegationCacheEntry() = default;
  DelegationCacheEntry(const DelegationCacheEntry&) = delete;
  DelegationCacheEntry(DelegationCacheEntry&&) = delete;
  DelegationCacheEntry& operator=(const DelegationCacheEntry&) = delete;

  DelegationInfo info;
  time_t ttd;
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
  const DelegationInfo* getBestFor(const DNSName& name) const;

  // TODO: serve stale?
  void pruneExpired();

private:
  SuffixMatchTree<DelegationCacheEntry> d_tree;
  mutable std::set<DNSName> d_toPrune;
};
} // namespace rec
