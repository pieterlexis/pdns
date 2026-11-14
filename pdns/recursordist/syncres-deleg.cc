#include "dns.hh"
#include "qtype.hh"
#include "rec-tcounters.hh"
#include "syncres.cc"
#include "syncres.hh"
#include <vector>

// HACK: We suffix all DELEG based delegations with this suffix
// for the "NS Name". This allows us to keep the NsSet struct
// around for now.
DNSName SyncRes::s_delegNameServerSuffix{"_d"};

/* ! Gets all address information from the cache for DELEG and DELEGPARAM records
 * This function implements a large part of draft-ietf-deleg-13 section 5.1.2
 *
 * @param record: The record we need to handle
 * @param flags: Any flags for the cache lookup
 * @param prefix: Spaces we need to prefix our loglines with
 * @param originalQName: QName of the initial, originating query. Used only for logging
 * @param originalDelegNama: There's a bunch of restrictions in the server-name and include-delegparam names based on the owner of the DELEG.
 * @param delegationInfo: This is where all the IP info (and info on missing records) ends up
 * @param limit: When we get a chain of included DELEGPARAMs, we decrement this before "recursing"
 * @return: Whether or not we were able to extract the required info from the record
 */
std::tuple<bool, vector<ComboAddress>> SyncRes::getIPAddressesFromDeleg(const DNSRecord& record, const MemRecursorCache::Flags& flags, const string& prefix, const DNSName& originalQName, const DNSName& originalDelegName, size_t limit)
{
  auto delegrr = getRR<DELEGBaseRecordContent>(record);
  if (delegrr == nullptr) {
    // We weren't passed a DELEG or DELEGPARAM
    return {false, {}};
  }

  limit--;
  if (limit == 0) {
    // Max depth reached
    return {false, {}};
  }

  auto allInfos = delegrr->getAllInfo();
  std::set<DelegInfo::DelegInfoKey> allKeys;

  // Step 1
  // Discard all DelegInfo elements with DelegInfoKey values that are
  // not supported by the resolver implementation. If no DelegInfo
  // elements remain after this filtering, stop processing the
  // record. Otherwise, continue using only the supported DelegInfo
  // elements.
  for (auto it = allInfos.begin(); it != allInfos.end(); it++) {
    if (it->getKey() > DelegInfo::DelegInfoKey::include_delegparam) {
      allInfos.erase(it);
      continue;
    }
    allKeys.insert(it->getKey());
  }
  if (allInfos.empty()) {
    return {false, {}};
  }

  // Step 2
  // If a DelegInfo element with the "mandatory" DelegInfoKey is
  // present, check its DelegInfoValue. The DelegInfoValue is a list
  // of keys which MUST have corresponding DelegInfo elements in the
  // record after the filtering in the previous steps. If any of the
  // listed DelegInfo elements is not present, stop processing this
  // record.
  if (auto info = delegrr->getInfo(DelegInfo::DelegInfoKey::mandatory); info != std::nullopt) {
    auto mandatoryKeys = info->getMandatory();
    if (!std::includes(allKeys.begin(), allKeys.end(), mandatoryKeys.begin(), mandatoryKeys.end())) {
      // There are keys in mandatory that are in the in record
      return {false, {}};
    }
  }

  // Step 3
  // If a record has more than one type of server information key
  // (excluding the IPv4/IPv6 case, see Section 3.4), or if it has
  // multiple server information keys of the same type, that record
  // is malformed. Stop processing this record.
  if (allKeys.count(DelegInfo::DelegInfoKey::server_ipv4) > 0 || allKeys.count(DelegInfo::DelegInfoKey::server_ipv6)) {
    if (allKeys.count(DelegInfo::DelegInfoKey::include_delegparam) > 0 || allKeys.count(DelegInfo::DelegInfoKey::server_name) > 0) {
      return {false, {}};
    }
  }

  if (allKeys.count(DelegInfo::DelegInfoKey::include_delegparam) > 0 && allKeys.count(DelegInfo::DelegInfoKey::server_name) > 0) {
    return {false, {}};
  }

  bool weDone{false};

  std::vector<ComboAddress> addresses;

  // Step 5
  // If server-ipv4 and/or server-ipv6 keys are present inside the
  // record, copy all of the address values into SLIST. Stop
  // processing this record.
  if (auto info = delegrr->getInfo(DelegInfo::DelegInfoKey::server_ipv4); info != std::nullopt && s_doIPv4) {
    addresses.insert(addresses.end(), info->getServerIPs().begin(), info->getServerIPs().end());
    weDone = true;
  }
  if (auto info = delegrr->getInfo(DelegInfo::DelegInfoKey::server_ipv6); info != std::nullopt && s_doIPv6) {
    addresses.insert(addresses.end(), info->getServerIPs().begin(), info->getServerIPs().end());
    weDone = true;
  }
  if (weDone) {
    return {true, addresses};
  }

  // Step 6
  // If a server-name key is present in the record, resolve each name
  // in the value into IPv4 and/or IPv6 addresses. Copy these
  // addresses into SLIST. Stop processing this record.
  if (auto info = delegrr->getInfo(DelegInfo::DelegInfoKey::server_name); info != std::nullopt) {
    for (const auto& name : info->getDnsNames()) {
      if (name.isPartOf(originalDelegName)) {
        // Step 4
        // If any DNS name referenced by server-name key or the include-
        // delegparam key is equal to or is a subdomain of the delegated
        // domain (i.e. the DELEG record owner), that record is malformed.
        // Stop processing this record.
        return {false, {}};
      }
      std::set<QType> nsNameAddressTypes;
      if (s_doIPv4 && !s_doIPv6) {
        nsNameAddressTypes.insert(QType::A);
      }
      if (s_doIPv6 && !s_doIPv4) {
        nsNameAddressTypes.insert(QType::AAAA);
      }

      vState state{vState::Indeterminate};
      vector<DNSRecord> addrRecords;
      for (const auto nsNameAddressType : nsNameAddressTypes) {
        auto ttd = g_recCache->get(d_now.tv_sec, name, nsNameAddressType, flags, &addrRecords, d_cacheRemote, d_routingTag, nullptr, nullptr, nullptr, &state, nullptr);
        if (ttd == -1 || g_now.tv_sec > ttd || !canUseRecords(prefix, name, record.d_name, nsNameAddressType, state)) {
          // Can't use for some reason, query for it!
          set<GetBestNSAnswer> beenThere;
          Context ctx;
          auto ret = doResolve(name, nsNameAddressType, addrRecords, 1, beenThere, ctx);
          if (ret != RCode::NoError) {
            // TODO: flawedNSSET
            continue;
          }
        } 
        for (const auto& addrRecord : addrRecords) {
          if (auto aRecordContent = getRR<ARecordContent>(addrRecord); aRecordContent != nullptr) {
            addresses.emplace_back(aRecordContent->getCA());
            continue;
          }
          if (auto aaaaRecordContent = getRR<AAAARecordContent>(addrRecord); aaaaRecordContent != nullptr) {
            addresses.emplace_back(aaaaRecordContent->getCA());
          }
        }
      }
    }
    return {true, addresses};
  }

  // Step 7
  // If an include-delegparam key is present in the record, resolve
  // each name in the value using the DELEGPARAM RR type. Recursively
  // apply the algorithm described in this section, after checking
  // that the maximum loop count described in Section 7.1 has not
  // been reached.
  if (auto info = delegrr->getInfo(DelegInfo::DelegInfoKey::include_delegparam); info != std::nullopt) {
    bool hadGoodOne{false};
    for (const auto& includeName : info->getDnsNames()) {
      if (includeName.isPartOf(originalDelegName)) {
        // Step 4
        // If any DNS name referenced by server-name key or the include-
        // delegparam key is equal to or is a subdomain of the delegated
        // domain (i.e. the DELEG record owner), that record is malformed.
        // Stop processing this record.
        return {false, {}};
      }
      vState state{vState::Indeterminate};
      vector<DNSRecord> delegParams;
      auto ttd = g_recCache->get(d_now.tv_sec, includeName, QType::DELEGPARAM, flags, &delegParams, d_cacheRemote, d_routingTag, nullptr, nullptr, nullptr, &state, nullptr);
      if (ttd == -1 || g_now.tv_sec > ttd || !canUseRecords(prefix, includeName, record.d_name, QType::DELEGPARAM, state)) {
          // Can't use for some reason, query for it!
          set<GetBestNSAnswer> beenThere;
          Context ctx;
          auto ret = doResolve(includeName, QType::DELEGPARAM, delegParams, 1, beenThere, ctx);
          if (ret != RCode::NoError) {
            continue;
          }
      }
      for (const auto& delegRecord : delegParams) {
        // XXX: Is includeName the correct one to pass, or should we pass originalDelegName?
        auto [success, includeAddresses] = getIPAddressesFromDeleg(delegRecord, flags, prefix + "  ", originalQName, includeName, limit);
        if (! success) {
          // TODO: something something flawednsset
          return {false, {}};
        }
        addresses.insert(addresses.end(), includeAddresses.begin(), includeAddresses.end());
      }
    }
    return {hadGoodOne, addresses};
  }
  return {false, {}};
}
