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

#include "ixfrdist.hh"
#include "ixfrdist-lmdb.hh"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include "lmdb-safe.hh"

namespace boost {
namespace serialization {

template<class Archive>
void save(Archive & ar, const DNSName& g, const unsigned int version)
{
  if(!g.empty()) {
    std::string tmp = g.toDNSStringLC(); // g++ 4.8 woes
    ar & tmp;
  }
  else
    ar & "";
}

template<class Archive>
void load(Archive & ar, DNSName& g, const unsigned int version)
{
  string tmp;
  ar & tmp;
  if(tmp.empty())
    g = DNSName();
  else
    g = DNSName(tmp.c_str(), tmp.size(), 0, false);
}

template<class Archive>
void save(Archive & ar, const QType& g, const unsigned int version)
{
  uint16_t tmp = g.getCode(); // g++ 4.8 woes
  ar & tmp;
}

template<class Archive>
void load(Archive & ar, QType& g, const unsigned int version)
{
  uint16_t tmp;
  ar & tmp; 
  g = QType(tmp);
}

template<class Archive>
void serialize(Archive & ar, DNSRecordContent& d, const unsigned int version)
{
  auto tmp = d.serialize(DNSName("."));
  ar & tmp;
}

template<class Archive>
void serialize(Archive & ar, DNSRecord& d, const unsigned int version)
{
  ar & d.d_name;
  ar & d.d_type;
  ar & d.d_ttl;
  ar & d.d_class;
  ar & d.d_clen;
  ar & *d.d_content;
}
} // namespace serialization
} // namespace boost

BOOST_SERIALIZATION_SPLIT_FREE(DNSName);
BOOST_SERIALIZATION_SPLIT_FREE(QType);
BOOST_IS_BITWISE_SERIALIZABLE(ComboAddress);

std::string serializeContent(uint16_t qtype, const DNSName& domain, const std::string& content)
{
  auto drc = DNSRecordContent::mastermake(qtype, 1, content);
  return drc->serialize(domain, false);
}

std::shared_ptr<DNSRecordContent> unserializeContentZR(uint16_t qtype, const DNSName& qname, const std::string& content)
{
  if(qtype == QType::A && content.size() == 4) {
    return std::make_shared<ARecordContent>(*((uint32_t*)content.c_str()));
  }
  return DNSRecordContent::unserialize(qname, qtype, content);
}

void IXFRDistDomain::doPutMeta(const string &keyname, const MDBInVal &val) const {
  auto txn = mdbenv->getRWTransaction();
  try {
    txn.put(metadb, keyname, val);
    txn.commit();
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("Could not write " + keyname + " for " + domain.toString() + " to the database: " + e.what());
  }
}

int IXFRDistDomain::doGetMeta(const string &keyname, MDBOutVal &val) const {
  auto txn = mdbenv->getROTransaction();
  int rc;
  try {
    rc = txn.get(metadb, keyname, val);
  } catch(const std::runtime_error &e) {
    throw std::runtime_error("Could not get metdata " + keyname + " for " + domain.toString() + " from the database: " + e.what());
  }
  return rc;
}

time_t IXFRDistDomain::getLastCheck() const {
  MDBOutVal val;
  int rc = doGetMeta(last_check_field_name, val);
  if (rc == MDB_NOTFOUND) {
    return 0;
  }
  return val.get<time_t>();
}

void IXFRDistDomain::setLastCheck(const time_t value) {
  MDBInVal val(value);
  doPutMeta(last_check_field_name, val);
}

shared_ptr<SOARecordContent> IXFRDistDomain::getSOA() const {
  MDBOutVal val;
  int rc = doGetMeta(soa_field_name, val);
  if (rc == MDB_NOTFOUND) {
    return nullptr;
  }
  // return std::make_shared<SOARecordContent>(val.get<SOARecordContent>());
  return nullptr;
}

void IXFRDistDomain::storeNewZoneVersion(const records_t &records) {
  shared_ptr<SOARecordContent> newSOA, oldSOA;
  uint32_t newSOATTL, oldSOATTL;
  getSOAFromRecords(records, newSOA, newSOATTL);

  auto txn = mdbenv->getRWTransaction();
  auto axfrcursor = txn.getCursor(axfrsdb);
  MDBOutVal oldSOAVal, oldAXFRVal;
  bool storeOnly = false;

  try {
    if (axfrcursor.get(oldSOAVal, oldAXFRVal, MDB_FIRST) == MDB_NOTFOUND) { // No previous AXFR exists
      storeOnly = true;
    }
  } catch(const std::runtime_error &e) {
    throw std::runtime_error("Unable to retrieve old AXFR for domain " + domain.toLogString() + ": " + e.what());
  }

  try {
    std::stringstream ss;
    boost::archive::text_oarchive oa{ss};
    oa<<records;
    txn.put(axfrsdb, newSOA->d_st.serial, ss.str());
    if (storeOnly) {
      txn.commit();
      return;
    }
  } catch(const std::runtime_error &e) {
    throw std::runtime_error("Unable to store new AXFR with serial " + std::to_string(newSOA->d_st.serial) + " for domain " + domain.toLogString() + ": " + e.what());
  }

  // There was an older AXFR, make IXFR diffs
  records_t oldAXFR;
  {
    std::stringstream iss;
    boost::archive::text_iarchive ia{iss};
    ia>>oldAXFR;
  }
  getSOAFromRecords(oldAXFR, oldSOA, oldSOATTL);

  try {
    MDBInVal oldSOAVal(oldSOA->d_st.serial);
    txn.del(axfrsdb, oldSOAVal);
  } catch(const std::runtime_error &e) {
    throw std::runtime_error("Unable to remove old AXFR for domain " + domain.toLogString() + ": " + e.what());
  }


  auto diff = std::make_shared<ixfrdiff_t>();
  makeIXFRDiff(oldAXFR, records, diff, oldSOA, oldSOATTL, newSOA, newSOATTL);
  auto ixfrval = MDBInVal::fromStruct(diff);
  try {
    txn.put(ixfrsdb, newSOA->d_st.serial, ixfrval);
    txn.commit();
  } catch(const std::runtime_error &e) {
    throw std::runtime_error("Unable to store new AXFR for domain " + domain.toLogString() + ": " + e.what());
  }
}
