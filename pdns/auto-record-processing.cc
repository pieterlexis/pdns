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
#include <stdexcept>
#include <vector>

#include "auto-record-processing.hh"
#include "dnsbackend.hh"

namespace pdns::auth::process_auto
{

static vector<DNSName> getNSNamesFor(const DNSName& target, const SOAData& sd) // NOLINT(readability-identifier-length)
{
  vector<DNSName> ret;
  sd.db->lookup(QType::NS, target, sd.domain_id);
  DNSZoneRecord rr; // NOLINT(readability-identifier-length)
  while (sd.db->get(rr)) {
    ret.push_back(getRR<NSRecordContent>(rr.dr)->getNS());
  }
  return ret;
}

static vector<ComboAddress> getIPAddressFor(const DNSName& target, const uint16_t qtype, const SOAData& sd) // NOLINT(readability-identifier-length)
{
  vector<ComboAddress> ret;
  if (qtype != QType::A && qtype != QType::AAAA) {
    return ret;
  }
  sd.db->lookup(qtype, target, sd.domain_id);
  DNSZoneRecord rr; // NOLINT(readability-identifier-length)
  while (sd.db->get(rr)) {
    if (qtype == QType::AAAA) {
      auto aaaarrc = getRR<AAAARecordContent>(rr.dr);
      ret.push_back(aaaarrc->getCA());
    }
    else if (qtype == QType::A) {
      auto arrc = getRR<ARecordContent>(rr.dr);
      ret.push_back(arrc->getCA());
    }
  }
  return ret;
}

void processDelegAuto(DNSZoneRecord& rec, SOAData& sd) // NOLINT(readability-identifier-length)
{
  auto rrc = getRR<DELEGBaseRecordContent>(rec.dr);
  if (!rrc->hasAuto()) {
    return;
  }
  auto newRRC = rrc->clone();
  if (!newRRC) {
    throw std::runtime_error("Creating a new record content failed for " + rec.dr.d_name.toLogString() + "|" + QType(rec.dr.d_type).toString());
  }
  auto names = getNSNamesFor(rec.dr.d_name, sd);

  {
    auto info = rrc->getInfo(DelegInfo::DelegInfoKey::server_name);
    if (info != std::nullopt && !info->isAuto()) {
      names = info->getDnsNames();
    }
  }

  for (const auto& delegInfoType : DelegInfo::getAutoKeys()) {
    auto info = rrc->getInfo(delegInfoType);
    if (info == std::nullopt || !info->isAuto()) {
      continue;
    }
    switch (delegInfoType) {
    case DelegInfo::DelegInfoKey::server_name: {
      // NOTE: we need to keep names around for the other DelegInfos
      auto movableNames = names;
      DelegInfo newInfo(DelegInfo::DelegInfoKey::server_name, std::move(movableNames), false);
      newRRC->setInfo(std::move(newInfo));
      break;
    }
    case DelegInfo::DelegInfoKey::server_ipv4: /* fallthrough */
    case DelegInfo::DelegInfoKey::server_ipv6: {
      if (names.empty()) {
        // XXX: This should be an error
        newRRC->removeInfo(delegInfoType);
        continue;
      }
      std::set<ComboAddress> addresses;
      for (const auto& name : names) {
        auto tmpAddresses = getIPAddressFor(name, delegInfoType == DelegInfo::DelegInfoKey::server_ipv4 ? QType::A : QType::AAAA, sd);
        addresses.insert(tmpAddresses.begin(), tmpAddresses.end());
      }
      if (addresses.empty()) {
        newRRC->removeInfo(delegInfoType);
        continue;
      }
      std::vector<ComboAddress> addressesIn{addresses.begin(), addresses.end()};
      DelegInfo newInfo(delegInfoType, std::move(addressesIn), false);
      newRRC->setInfo(std::move(newInfo));
      break;
    }
    default:
      // Not all AutoKeys are implemented! This is the programmer's error!
      throw std::logic_error("DelegInfo " + DelegInfo::keyToString(delegInfoType) + " auto processing is not implemented");
    }
  }
  rec.dr.setContent(std::move(newRRC));
}
}
