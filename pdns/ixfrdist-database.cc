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

uint32_t IXFRDistDatabase::getDomainSerial(const DNSName &d) {
  try {
    auto txn = lmdb::txn::begin(*getEnv(d), nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(txn, nullptr);

    lmdb::val serial;
    g_log<<Logger::Debug<<d_logPrefix<<__func__<<": retrieving serial for zone "<<d.toLogString()<<endl;
    if (!dbi.get(txn, lmdb::val("serial"), serial)) {
      // No exception but a false retval means "not found"
      txn.abort();
      throw PDNSException("Key not found in mdb");
    }
    auto retval = *serial.data<uint32_t>();
    txn.abort();
    g_log<<Logger::Debug<<d_logPrefix<<__func__<<": had serial '"<<retval<<"' for zone "<<d.toLogString()<<endl;
    return retval;
  }
  catch(const lmdb::error &e) {
    throw PDNSException(std::string("Could not get serial from database: ") + e.what());
  }
}

void IXFRDistDatabase::setDomainSerial(const DNSName &d, const uint32_t &serial) {
  try {
    auto txn = lmdb::txn::begin(*getEnv(d));
    auto dbi = lmdb::dbi::open(txn, nullptr);

    g_log<<Logger::Debug<<d_logPrefix<<__func__<<": Storing serial "<<serial<<" for zone "<<d.toLogString()<<endl;
    dbi.put(txn, "serial", serial, lmdb::dbi::default_put_flags);
    txn.commit();
  }
  catch(const lmdb::error &e) {
    throw PDNSException(std::string("Could set serial in database: ") + e.what());
  }
}
