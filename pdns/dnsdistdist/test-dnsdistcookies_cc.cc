#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_NO_MAIN

#include <boost/test/unit_test.hpp>
#include "dnsdist-cookies.hh"
#include "misc.hh"

BOOST_AUTO_TEST_SUITE(test_dnsdistcookies_cc)
BOOST_AUTO_TEST_CASE(test_Simple) {
  auto clientCookie = makeBytesFromHex("a05862bf552bbd22"); // From draft-sury-toorop-dns-cookies-algorithms-00
  time_t t = 1559731985; // Wed Jun 5 10:53:05 UTC 2019 from draft-sury-toorop-dns-cookies-algorithms-00
  auto toCheck = makeBytesFromHex("a05862bf552bbd22010000005cf79f11de1bba0952b0ff82"); // From draft-sury-toorop-dns-cookies-algorithms-00
  ComboAddress source("198.51.100.100"); // From draft-sury-toorop-dns-cookies-algorithms-00
  auto key = "e5e973e5a6b2a43f48e7dc849e37bfcf"; // From draft-sury-toorop-dns-cookies-algorithms-00

  auto ctx = DNSDistCookieContext(key);
  auto ret = ctx.makeCookie(clientCookie, source, &t);
  BOOST_CHECK_EQUAL(ret, toCheck);
  BOOST_CHECK(ctx.isCorrectCookie(ret, source));
}
BOOST_AUTO_TEST_SUITE_END()
