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

#include <string>
#include "ixfrdist-database.hh"
#include "pdnsexception.hh"
#include "misc.hh"

IXFRDistDatabase::IXFRDistDatabase(const std::string& fpath) {
  try {
    d_env = make_unique<lmdb::env>(lmdb::env::create());
    d_env->set_mapsize(1UL * 1024UL * 1024UL * 1024UL); // 1 GiB
    d_env->open(fpath.c_str(), 0, 0664);
  }
  catch(const lmdb::error &e) {
    throw PDNSException(std::string("Could not create LMDB environment: ") + e.what());
  }
}
