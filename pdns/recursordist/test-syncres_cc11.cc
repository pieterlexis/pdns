#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#include "dnsname.hh"
#include "qtype.hh"
#include "rec-tcounters.hh"
#endif

#include <boost/test/unit_test.hpp>

#include "test-syncres_cc.hh"
#include "taskqueue.hh"
#include "rec-taskqueue.hh"

BOOST_AUTO_TEST_SUITE(syncres_cc11)
BOOST_AUTO_TEST_CASE(testDelegServerName)
{
  std::unique_ptr<SyncRes> sr;
  initSR(sr);
  primeHints();

  const DNSName target("foobar.deleg.example.");

  sr->setAsyncCallback([&](const ComboAddress& address, const DNSName& domain, int type, bool /* doTCP */, bool /* sendRDQuery */, int /* EDNS0Level */, struct timeval* /* now */, std::optional<Netmask>& /* srcmask */, const ResolveContext& /* context */, LWResult* res, bool* /* chained */) {
    if (isRootServer(address)) {
      if (domain.isPartOf(DNSName("example."))) {
        setLWResult(res, 0, false, false, true);
        addRecordToLW(res, "example.", QType::NS, "a.gtld-servers.net.", DNSResourceRecord::AUTHORITY, 172800);
        addRecordToLW(res, "a.gtld-servers.net.", QType::A, "192.0.2.1", DNSResourceRecord::ADDITIONAL, 3600);
        return LWResult::Result::Success;
      }
    }
    if (address == ComboAddress("192.0.2.1:53")) {
      if (domain.isPartOf(DNSName("deleg.example."))) {
        setLWResult(res, 0, false, false, true);
        addRecordToLW(res, "deleg.example.", QType::DELEG, "server-name=ns1.hoster.example.", DNSResourceRecord::AUTHORITY, 172800);
        return LWResult::Result::Success;
      }
      if (domain.isPartOf(DNSName("hoster.example."))) {
        setLWResult(res, 0, false, false, true);
        addRecordToLW(res, "hoster.example.", QType::NS, "ns1.hoster.example.", DNSResourceRecord::AUTHORITY, 172800);
        addRecordToLW(res, "ns1.hoster.example.", QType::A, "192.0.2.3", DNSResourceRecord::ADDITIONAL, 3600);
        return LWResult::Result::Success;
      }
    }
    if (address == ComboAddress("192.0.2.3:53")) {
      if (domain == DNSName("ns1.hoster.example.") && type == QType::A) {
        setLWResult(res, 0, true, false, true);
        addRecordToLW(res, domain, QType::A, "192.0.2.3", DNSResourceRecord::ANSWER, 3600);
        return LWResult::Result::Success;
      }
      if (domain == target && type == QType::A) {
        setLWResult(res, 0, true, false, true);
        addRecordToLW(res, domain, QType::A, "203.0.113.1", DNSResourceRecord::ANSWER, 3600);
        return LWResult::Result::Success;
      }
      setLWResult(res, RCode::NXDomain, true, false, true);
      return LWResult::Result::Success;
    }
    return LWResult::Result::Timeout;
  });

  vector<DNSRecord> ret;
  int res = sr->beginResolve(target, QType::A, QClass::IN, ret);
  BOOST_CHECK_EQUAL(res, RCode::NoError);
  BOOST_REQUIRE_EQUAL(ret.size(), 1U);
  BOOST_CHECK(ret[0].d_type == QType::A);
  BOOST_CHECK_EQUAL(ret[0].d_name, target);
}
BOOST_AUTO_TEST_SUITE_END()
