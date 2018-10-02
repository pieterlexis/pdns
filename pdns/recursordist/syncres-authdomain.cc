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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "syncres.hh"

//! This is the 'out of band resolver', in other words, the authoritative server
void SyncRes::AuthDomain::addSOA(std::vector<DNSRecord>& records) const
{
  SyncRes::AuthDomain::records_t::const_iterator ziter = d_records.find(boost::make_tuple(getName(), QType::SOA));
  if (ziter != d_records.end()) {
    DNSRecord dr = *ziter;
    dr.d_place = DNSResourceRecord::AUTHORITY;
    records.push_back(dr);
  }
  else {
    // cerr<<qname<<": can't find SOA record '"<<getName()<<"' in our zone!"<<endl;
  }
}

int SyncRes::AuthDomain::getRecords(const DNSName& qname, uint16_t qtype, std::vector<DNSRecord>& records) const
{
  int result = RCode::NoError;
  records.clear();

  // partial lookup
  std::pair<records_t::const_iterator,records_t::const_iterator> range = d_records.equal_range(tie(qname));

  SyncRes::AuthDomain::records_t::const_iterator ziter;
  bool somedata = false;

  for(ziter = range.first; ziter != range.second; ++ziter) {
    somedata = true;

    if(qtype == QType::ANY || ziter->d_type == qtype || ziter->d_type == QType::CNAME) {
      // let rest of nameserver do the legwork on this one
      records.push_back(*ziter);
    }
    else if (ziter->d_type == QType::NS && ziter->d_name.countLabels() > getName().countLabels()) {
      // we hit a delegation point!
      DNSRecord dr = *ziter;
      dr.d_place=DNSResourceRecord::AUTHORITY;
      records.push_back(dr);
    }
  }

  if (!records.empty()) {
    /* We have found an exact match, we're done */
    // cerr<<qname<<": exact match in zone '"<<getName()<<"'"<<endl;
    return result;
  }

  if (somedata) {
    /* We have records for that name, but not of the wanted qtype */
    // cerr<<qname<<": found record in '"<<getName()<<"', but nothing of the right type, sending SOA"<<endl;
    addSOA(records);

    return result;
  }

  // cerr<<qname<<": nothing found so far in '"<<getName()<<"', trying wildcards"<<endl;
  DNSName wcarddomain(qname);
  while(wcarddomain != getName() && wcarddomain.chopOff()) {
    // cerr<<qname<<": trying '*."<<wcarddomain<<"' in "<<getName()<<endl;
    range = d_records.equal_range(boost::make_tuple(g_wildcarddnsname + wcarddomain));
    if (range.first==range.second)
      continue;

    for(ziter = range.first; ziter != range.second; ++ziter) {
      DNSRecord dr = *ziter;
      // if we hit a CNAME, just answer that - rest of recursor will do the needful & follow
      if(dr.d_type == qtype || qtype == QType::ANY || dr.d_type == QType::CNAME) {
        dr.d_name = qname;
        dr.d_place = DNSResourceRecord::ANSWER;
        records.push_back(dr);
      }
    }

    if (records.empty()) {
      addSOA(records);
    }

    // cerr<<qname<<": in '"<<getName()<<"', had wildcard match on '*."<<wcarddomain<<"'"<<endl;
    return result;
  }

  /* Nothing for this name, no wildcard, let's see if there is some NS */
  DNSName nsdomain(qname);
  while (nsdomain.chopOff() && nsdomain != getName()) {
    range = d_records.equal_range(boost::make_tuple(nsdomain,QType::NS));
    if(range.first == range.second)
      continue;

    for(ziter = range.first; ziter != range.second; ++ziter) {
      DNSRecord dr = *ziter;
      dr.d_place = DNSResourceRecord::AUTHORITY;
      records.push_back(dr);
    }
  }

  if(records.empty()) {
    // cerr<<qname<<": no NS match in zone '"<<getName()<<"' either, handing out SOA"<<endl;
    addSOA(records);
    result = RCode::NXDomain;
  }

  return result;
}

bool SyncRes::doOOBResolve(const AuthDomain& domain, const DNSName &qname, const QType &qtype, vector<DNSRecord>&ret, int& res)
{
  d_authzonequeries++;
  s_authzonequeries++;

  res = domain.getRecords(qname, qtype.getCode(), ret);
  return true;
}
