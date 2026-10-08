#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif

#define BOOST_TEST_NO_MAIN

#include <boost/test/unit_test.hpp>
#include "qtype.hh"

using namespace boost;

BOOST_AUTO_TEST_SUITE(test_qtype_hh)
BOOST_AUTO_TEST_CASE(deleg)
{
  BOOST_CHECK(!QType::isDelegationType(0xF000 - 1));
  BOOST_CHECK(QType::isDelegationType(0xF000));
  BOOST_CHECK(QType::isDelegationType(0xF1FF));
  BOOST_CHECK(!QType::isDelegationType(0xF1FF + 1));

  BOOST_CHECK(QType::isDelegationType(QType::NS, true));
  BOOST_CHECK(!QType::isDelegationType(QType::NS, false));

  BOOST_CHECK(!QType::isNsPreservingDelegationType(0xF080-1));
  BOOST_CHECK(QType::isNsPreservingDelegationType(0xF080));
  BOOST_CHECK(QType::isNsPreservingDelegationType(0xF0FF));
  BOOST_CHECK(!QType::isNsPreservingDelegationType(0xF0FF+1));

  // Private types are NS-preserving
  BOOST_CHECK(!QType::isNsPreservingDelegationType(0xF1F0-1));
  BOOST_CHECK(QType::isNsPreservingDelegationType(0xF1F0));
  BOOST_CHECK(QType::isNsPreservingDelegationType(0xF1FF));
  BOOST_CHECK(!QType::isNsPreservingDelegationType(0xF1FF+1));

  BOOST_CHECK(!QType::isNsOmittingDelegationType(0xF000-1));
  BOOST_CHECK(QType::isNsOmittingDelegationType(0xF000));
  BOOST_CHECK(QType::isNsOmittingDelegationType(0xF07F));
  BOOST_CHECK(!QType::isNsOmittingDelegationType(0xF07F+1));
}
BOOST_AUTO_TEST_SUITE_END()
