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
#include "ext/lmdbxx/lmdb++.h"
#include "dnsname.hh"
#include "misc.hh"

class IXFRDistDatabase
{
  public:
    explicit IXFRDistDatabase(const std::string& fpath) : d_workDir(fpath) {};

    uint32_t getDomainSerial(const DNSName &d);
    void setDomainSerial(const DNSName &d, const uint32_t &serial);

  private:
    std::string d_workDir;
    std::map<DNSName, shared_ptr<lmdb::env>> d_envs;

    // const lmdb::val d_keySerial = "serial";

    std::shared_ptr<lmdb::env> getEnv(const DNSName &d) {
      auto it = d_envs.find(d);
      if (it != d_envs.end()) {
        return it->second;
      }
      auto fname = d_workDir + "/" + d.toString();

      auto env = lmdb::env::create();
      env.set_mapsize(1UL * 1024UL * 1024UL * 1024UL); // 1GB
      env.open(fname.c_str(), 0, 0664);
      d_envs[d] = std::make_shared<lmdb::env>(std::move(env));
      return d_envs[d]; // TODO don't do this lookup again
    };
};
