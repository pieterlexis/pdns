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
#include <iostream>
#include <boost/format.hpp>

#include "dnsrulactions.hh"
#include "dnsrecords.hh"

using namespace std;

bool ProbaRule::matches(const DNSQuestion* dq) const
{
  if(d_proba == 1.0)
    return true;
  double rnd = 1.0*random() / RAND_MAX;
  return rnd > (1.0 - d_proba);
}

string ProbaRule::toString() const 
{
  return "match with prob. " + (boost::format("%0.2f") % d_proba).str();
}


TeeAction::TeeAction(const ComboAddress& ca, bool addECS) : d_remote(ca), d_addECS(addECS)
{
  d_fd=SSocket(d_remote.sin4.sin_family, SOCK_DGRAM, 0);
  SConnect(d_fd, d_remote);
  setNonBlocking(d_fd);
  d_worker=std::thread(std::bind(&TeeAction::worker, this));  
}

TeeAction::~TeeAction()
{
  d_pleaseQuit=true;
  close(d_fd);
  d_worker.join();
}


DNSAction::Action TeeAction::operator()(DNSQuestion* dq, string* ruleresult) const 
{
  if(dq->tcp) {
    d_tcpdrops++;
  }
  else {
    ssize_t res;
    d_queries++;

    if(d_addECS) {
      std::string query;
      uint16_t len = dq->len;
      bool ednsAdded = false;
      bool ecsAdded = false;
      query.reserve(dq->size);
      query.assign((char*) dq->dh, len);

      if (!handleEDNSClientSubnet((char*) query.c_str(), query.capacity(), dq->qname->wirelength(), &len, &ednsAdded, &ecsAdded, *dq->remote, dq->ecsOverride, dq->ecsPrefixLength)) {
        return DNSAction::Action::None;
      }

      res = send(d_fd, query.c_str(), len, 0);
    }
    else {
      res = send(d_fd, (char*)dq->dh, dq->len, 0);
    }

    if (res <= 0)
      d_senderrors++;
  }
  return DNSAction::Action::None;
}

string TeeAction::toString() const
{
  return "tee to "+d_remote.toStringWithPort();
}

std::unordered_map<string,double> TeeAction::getStats() const
{
  return {{"queries", d_queries},
          {"responses", d_responses},
          {"recv-errors", d_recverrors},
          {"send-errors", d_senderrors},
          {"noerrors", d_noerrors},
          {"nxdomains", d_nxdomains},
          {"refuseds", d_refuseds},
          {"servfails", d_servfails},
          {"other-rcode", d_otherrcode},
          {"tcp-drops", d_tcpdrops}
  };
}

void TeeAction::worker()
{
  char packet[1500];
  int res=0;
  struct dnsheader* dh=(struct dnsheader*)packet;
  for(;;) {
    res=waitForData(d_fd, 0, 250000);
    if(d_pleaseQuit)
      break;
    if(res < 0) {
      usleep(250000);
      continue;
    }
    if(res==0)
      continue;
    res=recv(d_fd, packet, sizeof(packet), 0);
    if(res <= (int)sizeof(struct dnsheader)) 
      d_recverrors++;
    else if(res > 0)
      d_responses++;

    if(dh->rcode == RCode::NoError)
      d_noerrors++;
    else if(dh->rcode == RCode::ServFail)
      d_servfails++;
    else if(dh->rcode == RCode::NXDomain)
      d_nxdomains++;
    else if(dh->rcode == RCode::Refused)
      d_refuseds++;
    else if(dh->rcode == RCode::FormErr)
      d_formerrs++;
    else if(dh->rcode == RCode::NotImp)
      d_notimps++;
  }
}

DNSAction::Action RewriteMapAction::operator()(DNSQuestion* dq, string* ruleresult) const
{
  const auto pair = d_smt.lookup(*dq->qname);

  if (pair == nullptr) {
    return Action::None;
  }

  DNSName rebased(*dq->qname);
  rebased.rebase(pair->first, pair->second);
  char* buffer = reinterpret_cast<char*>(dq->dh);
  const size_t originalQNameSize = dq->qname->wirelength();
  const std::string newQNameWire =  rebased.toDNSString();
  const size_t newQNameSize = newQNameWire.size();
  size_t newSize = dq->len - originalQNameSize + newQNameSize;

  if (dq->len < originalQNameSize || newSize > dq->size) {
    return Action::None;
  }

  if (dq->len > (sizeof(dnsheader) + originalQNameSize)) {
    const size_t remainingDataSize = dq->len - (sizeof(dnsheader) + originalQNameSize);
    memmove(buffer + sizeof(dnsheader) + newQNameSize, buffer + sizeof(dnsheader) + originalQNameSize, remainingDataSize);
  }

  memcpy(buffer + sizeof(dnsheader), newQNameWire.data(), newQNameSize);

  dq->len = newSize;
  *(dq->qname) = rebased;

  return Action::HeaderModify;
}

static void rebaseContent(const uint16_t qtype, std::shared_ptr<DNSRecordContent>& content, const DNSName& from, const DNSName& to)
{
  if (qtype == QType::CNAME) {
    const auto initial = std::dynamic_pointer_cast<CNAMERecordContent>(content);
    if (!initial) {
      return;
    }
    DNSName rebased(initial->getTarget());
    if (!rebased.isPartOf(from)) {
      return;
    }
    rebased.rebase(from, to);
    content = std::make_shared<CNAMERecordContent>(rebased);
  }
  else if (qtype == QType::MX) {
    const auto initial = std::dynamic_pointer_cast<MXRecordContent>(content);
    if (!initial) {
      return;
    }
    DNSName rebased(initial->d_mxname);
    if (!rebased.isPartOf(from)) {
      return;
    }
    rebased.rebase(from, to);
    auto rebasedContent = std::make_shared<MXRecordContent>(initial->d_preference, rebased);
    content = rebasedContent;
  }
  else if (qtype == QType::SRV) {
    const auto initial = std::dynamic_pointer_cast<SRVRecordContent>(content);
    if (!initial) {
      return;
    }
    DNSName rebased(initial->d_target);
    if (!rebased.isPartOf(from)) {
      return;
    }
    rebased.rebase(from, to);
    auto rebasedContent = std::make_shared<SRVRecordContent>(initial->d_preference, initial->d_weight, initial->d_port, rebased);
    content = rebasedContent;
  }
  else if (qtype == QType::NS) {
    const auto initial = std::dynamic_pointer_cast<NSRecordContent>(content);
    if (!initial) {
      return;
    }
    DNSName rebased(initial->getNS());
    if (!rebased.isPartOf(from)) {
      return;
    }
    rebased.rebase(from, to);
    auto rebasedContent = std::make_shared<NSRecordContent>(rebased);
    content = rebasedContent;
  }
}

static bool rewriteAnswer(DNSResponse& dr, const DNSName& from, const DNSName& to)
{
  try {
    MOADNSParser mdp(false, reinterpret_cast<const char*>(dr.dh), dr.len);

    vector<uint8_t> packet;
    DNSName newQName(*dr.qname);
    newQName.rebase(from, to);

    DNSPacketWriter pw(packet, newQName, mdp.d_qtype, mdp.d_qclass);

    *(pw.getHeader()) = mdp.d_header;
    pw.getHeader()->qdcount = htons(1);
    pw.getHeader()->ancount = 0;
    pw.getHeader()->nscount = 0;
    pw.getHeader()->arcount = 0;

    for(auto& pair : mdp.d_answers) {
      DNSRecord& dr = pair.first;

      DNSName rebased(dr.d_name);
      if (rebased.isPartOf(from)) {
        rebased.rebase(from, to);
      }

      pw.startRecord(rebased, dr.d_type, dr.d_ttl, dr.d_class, dr.d_place);

      if (dr.d_class == QClass::IN) {
        rebaseContent(dr.d_type, dr.d_content, from, to);
      }

      dr.d_content->toPacket(pw);
    }
    pw.commit();

    if (packet.size() > dr.size) {
      return false;
    }

    memcpy(reinterpret_cast<char*>(dr.dh), packet.data(), packet.size());
    dr.len = packet.size();

    return true;
  }
  catch(const std::exception& e) {
    vinfolog("Got exception while rewriting answer: %s", e.what());
  }

  return false;
}

DNSResponseAction::Action RewriteMapResponseAction::operator()(DNSResponse* dr, string* ruleresult) const
{
  const auto pair = d_smt.lookup(*dr->qname);

  if (pair == nullptr) {
    return Action::None;
  }

  if (rewriteAnswer(*dr, pair->first, pair->second)) {
    return Action::HeaderModify;
  }
  return Action::None;
}
