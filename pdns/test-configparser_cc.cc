#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_NO_MAIN
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/test/unit_test.hpp>
#include "configparser.hh"

BOOST_AUTO_TEST_SUITE(test_configparser_cc)

BOOST_AUTO_TEST_CASE(test_declare_bool) {
  Config c;
  c.declareItem("mybool", Config::ConfigItemType::Bool, "some description", "some help", false, false, true);
  BOOST_CHECK(c.getBool("mybool"));

  BOOST_CHECK_THROW(c.declareItem("mybrokenbool", Config::ConfigItemType::Bool, "some description", "some help", false, false, "foo"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END();
