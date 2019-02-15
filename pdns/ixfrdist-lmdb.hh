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
#include "dnsname.hh"
#include "dnsrecords.hh"
#include "lmdb-safe.hh"
#include "ixfrutils.hh"
#include <cstdlib>

/* This class represents a domain that is distributed by ixfrdist, it contains the
 * configuration and data
 */
class IXFRDistDomain
{
  public:
    explicit IXFRDistDomain(const DNSName &zone, const string &workdir) {
      domain = zone;
      string p;
      if (realpath(workdir.c_str(), &p[0]) == nullptr) {
        throw PDNSException("Could not determine work directory: " + stringerror());
      }
      mdbenv = getMDBEnv((p + "/" + domain.toStringRootROOT()).c_str(), 0, 0600);
      metadb = mdbenv->openDB("meta", MDB_CREATE);
      axfrsdb = mdbenv->openDB("axfrs", MDB_CREATE);
      ixfrsdb = mdbenv->openDB("ixfrs", MDB_CREATE);
    };
    ~IXFRDistDomain() {};

    // Returns the SOA from the database or a nullptr
    shared_ptr<SOARecordContent> getSOA() const;

    void addMaster(const ComboAddress &address) { masters.insert(address); };
    set<ComboAddress> getMasters() const { return masters; };
    ComboAddress getRandomMaster() const {
      auto it(masters.begin());
      std::advance(it, random() % masters.size());
      return ComboAddress(*it);
    };

    DNSName getDomain() const { return domain; };

    time_t getLastCheck() const;
    void setLastCheck(const time_t value);

    void storeNewZoneVersion(const records_t &records);

  private:
    DNSName domain;
    set<ComboAddress> masters; // A set so we can do multiple master addresses in the future

    shared_ptr<MDBEnv> mdbenv;
    MDBDbi metadb;
    MDBDbi axfrsdb;
    MDBDbi ixfrsdb;

    MDBOutVal doGetMeta(const string &keyname) const;
    void doPutMeta(const string &keyname, const MDBInVal &val) const;

    // All the fieldnames we'll use
    const string last_check_field_name = "last_check";
    const string soa_field_name = "soa";
};
