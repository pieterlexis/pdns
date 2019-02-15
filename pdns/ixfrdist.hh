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
#include "dnsrecords.hh"
#include "ixfrutils.hh"

struct ixfrdiff_t {
  shared_ptr<SOARecordContent> oldSOA;
  shared_ptr<SOARecordContent> newSOA;
  vector<DNSRecord> removals;
  vector<DNSRecord> additions;
  uint32_t oldSOATTL;
  uint32_t newSOATTL;
};

struct ixfrinfo_t {
  shared_ptr<SOARecordContent> soa; // The SOA of the latest AXFR
  records_t latestAXFR;             // The most recent AXFR
  vector<std::shared_ptr<ixfrdiff_t>> ixfrDiffs;
  uint32_t soaTTL;
};

// Why a struct? This way we can add more options to a domain in the future
struct ixfrdistdomain_t {
  set<ComboAddress> masters; // A set so we can do multiple master addresses in the future
};

static void getSOAFromRecords(const records_t& records, shared_ptr<SOARecordContent>& soa, uint32_t& soaTTL) {
  for (const auto& dnsrecord : records) {
    if (dnsrecord.d_type == QType::SOA) {
      soa = getRR<SOARecordContent>(dnsrecord);
      if (soa == nullptr) {
        throw PDNSException("Unable to determine SOARecordContent from records");
      }
      soaTTL = dnsrecord.d_ttl;
      return;
    }
  }
  throw PDNSException("No SOA in supplied records");
}

static void makeIXFRDiff(const records_t& from, const records_t& to, std::shared_ptr<ixfrdiff_t>& diff, const shared_ptr<SOARecordContent>& fromSOA = nullptr, uint32_t fromSOATTL=0, const shared_ptr<SOARecordContent>& toSOA = nullptr, uint32_t toSOATTL = 0) {
  set_difference(from.cbegin(), from.cend(), to.cbegin(), to.cend(), back_inserter(diff->removals), from.value_comp());
  set_difference(to.cbegin(), to.cend(), from.cbegin(), from.cend(), back_inserter(diff->additions), from.value_comp());
  diff->oldSOA = fromSOA;
  diff->oldSOATTL = fromSOATTL;
  if (fromSOA == nullptr) {
    getSOAFromRecords(from, diff->oldSOA, diff->oldSOATTL);
  }
  diff->newSOA = toSOA;
  diff->newSOATTL = toSOATTL;
  if (toSOA == nullptr) {
    getSOAFromRecords(to, diff->newSOA, diff->newSOATTL);
  }
}
