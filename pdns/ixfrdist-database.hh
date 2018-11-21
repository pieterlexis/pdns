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
#include <memory>
#include <sys/stat.h>
#include "ixfrdist.hh"
#include "ext/lmdbxx/lmdb++.h"
#include "dnsname.hh"
#include "logger.hh"

class IXFRDistDatabase
{
  public:
    explicit IXFRDistDatabase(const std::string& fpath) : d_workDir(fpath) {};

    uint32_t getDomainSerial(const DNSName &d);
    void setDomainSerial(const DNSName &d, const uint32_t &serial);

    records_t getZone(const DNSName &d, lmdb::txn *parentTxn);
    void updateZone(const DNSName &d, const records_t &records);

  private:
    std::string d_logPrefix{"[database] "};
    std::string d_workDir;
    std::map<DNSName, shared_ptr<lmdb::env>> d_envs;

    std::shared_ptr<lmdb::env> getEnv(const DNSName &d) {
      auto it = d_envs.find(d);
      if (it != d_envs.end()) {
        return it->second;
      }

      auto fname = d_workDir + "/" + d.toString() + "db";
      if (d.isRoot()) {
        fname = d_workDir + "/" + "ROOT" + ".db";
      }

      auto env = lmdb::env::create();
      env.set_mapsize(1UL * 1024UL * 1024UL * 1024UL); // 1GB
      env.open(fname.c_str(), MDB_NOSUBDIR, 0664);
      auto e = std::make_shared<lmdb::env>(std::move(env));
      d_envs[d] = e;
      return e;
    };

    std::string dumpZone(const records_t &records) {
      ostringstream data;
      for (auto const &r : records) {
        data<<(r.d_name.isRoot() ? "@" :  r.d_name.toStringNoDot())
          <<"\t"<<r.d_ttl<<"\t"<<DNSRecordContent::NumberToType(r.d_type)
          <<"\t"<<r.d_content->getZoneRepresentation()<<endl;
      }
      return data.str();
    };
};
