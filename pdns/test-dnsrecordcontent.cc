#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_NO_MAIN
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/test/unit_test.hpp>
#include <stdexcept>
#include "dnsrecords.hh"
#include "iputils.hh"

BOOST_AUTO_TEST_SUITE(test_dnsrecordcontent)

BOOST_AUTO_TEST_CASE(test_equality) {
  ComboAddress ip("1.2.3.4"), ip2("10.0.0.1"), ip6("::1");
  ARecordContent a1(ip), a2(ip), a3(ip2);
  AAAARecordContent aaaa(ip6), aaaa1(ip6);
  
  BOOST_CHECK(a1.operator==(a2));
  BOOST_CHECK(!(a1.operator==(a3)));

  BOOST_CHECK(aaaa.operator==(aaaa1));

  auto rec1 = DNSRecordContent::make(QType::A, 1, "192.168.0.1");
  auto rec2 = DNSRecordContent::make(QType::A, 1, "192.168.222.222");
  auto rec3 = DNSRecordContent::make(QType::AAAA, 1, "::1");
  auto recMX = DNSRecordContent::make(QType::MX, 1, "25 smtp.powerdns.com");
  auto recMX2 = DNSRecordContent::make(QType::MX, 1, "26 smtp.powerdns.com");
  auto recMX3 = DNSRecordContent::make(QType::MX, 1, "26 SMTP.powerdns.com");
  BOOST_CHECK(!(*rec1==*rec2));
  BOOST_CHECK(*rec1==*rec1);
  BOOST_CHECK(*rec3==*rec3);

  BOOST_CHECK(*recMX==*recMX);
  BOOST_CHECK(*recMX2==*recMX3);
  BOOST_CHECK(!(*recMX==*recMX3));
  
  
  BOOST_CHECK(!(*rec1==*rec3));

  NSRecordContent ns1(DNSName("ns1.powerdns.com")), ns2(DNSName("NS1.powerdns.COM")), ns3(DNSName("powerdns.net"));
  BOOST_CHECK(ns1.operator==(ns2));
  BOOST_CHECK(!(ns1.operator==(ns3)));
}

BOOST_AUTO_TEST_CASE(test_DELEG) {
  std::vector<std::string> validRecords{
    "server-ipv4=192.0.2.1",
    "mandatory=server-ipv4 server-ipv4=192.0.2.1",
    "server-ipv6=2001:db8::1",
    "mandatory=server-ipv6 server-ipv6=2001:db8::1",
    "server-ipv4=192.0.2.1 server-ipv6=2001:db8::1",
    "mandatory=server-ipv4 server-ipv4=192.0.2.1 server-ipv6=2001:db8::1",
    "mandatory=server-ipv4,server-ipv6 server-ipv4=192.0.2.1 server-ipv6=2001:db8::1",
    "include-delegparam=foo.example.",
    "mandatory=include-delegparam include-delegparam=foo.example.",
  };

  for (const auto& record : validRecords) {
    DELEGRecordContent delegRecord(record);
    BOOST_CHECK_MESSAGE(delegRecord.mandatoryIsComplete(false), "mandatory is not valid for record '"<<record<<"'");
    BOOST_CHECK_MESSAGE(delegRecord.isValid(false), "record '"<<record<<"' is not valid!");
  }

  std::vector<std::string> invalidRecords{
    "mandatory=server-name", // missing info from mandatory
    "server-ipv4=192.0.2.1 server-name=foo.example.", // forbidden together
    "mandatory=server-ipv4,server-ipv6 server-ipv4=192.0.2.1", // missing info from mandatory
    "server-ipv4=192.0.2.1 server-ipv6=2001:db8::1 server-name=foo.example.", // forbidden together
    "include-delegparam=foo.example. server-ipv6=2001:db8::1", // forbidden together
    "server-ipv4=192.0.2.1 server-ipv6=2001:db8::1 include-delegparam=foo.example.", // forbidden together
  };

  for (const auto& record : invalidRecords) {
    DELEGRecordContent delegRecord(record);
    BOOST_CHECK_MESSAGE(!delegRecord.isValid(false), "invalid record '"<<record<<"' is marked valid!");
    BOOST_CHECK_THROW(delegRecord.isValid(true), std::invalid_argument);
  }
}

BOOST_AUTO_TEST_SUITE_END()
