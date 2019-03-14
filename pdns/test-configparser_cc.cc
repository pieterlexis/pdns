#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_NO_MAIN
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <boost/test/unit_test.hpp>
#include "configparser.hh"

BOOST_AUTO_TEST_SUITE(test_configparser_cc)

BOOST_AUTO_TEST_CASE(test_declare_missing_fields) {
  Config c;
  BOOST_CHECK_THROW(c.declareItem("", Config::ConfigItemType::Bool, "some description", "some help", false, false, true), std::runtime_error); // No name
  BOOST_CHECK_THROW(c.declareItem("abool", Config::ConfigItemType::Bool, "", "some help", false, false, true), std::runtime_error); // No description
  BOOST_CHECK_THROW(c.declareItem("abool", Config::ConfigItemType::Bool, "desc", "", false, false, true), std::runtime_error); // No help

  BOOST_CHECK_THROW(c.declareItem("abool", Config::ConfigItemType::Bool, "desc", "help", false, false, nullptr), std::runtime_error); // empty value
  BOOST_CHECK_THROW(c.declareItem("abool", Config::ConfigItemType::Bool, "desc", "help", false, false, boost::any()), std::runtime_error); // empty value

  c.declareItem("abool", Config::ConfigItemType::Bool, "desc", "help", false, false, true);
  BOOST_CHECK_THROW(c.declareItem("abool", Config::ConfigItemType::Bool, "desc", "", false, false, true), std::runtime_error); // Already declared
}

BOOST_AUTO_TEST_CASE(test_declare_bool) {
  Config c;
  c.declareItem("mybool", Config::ConfigItemType::Bool, "some description", "some help", false, false, true);
  BOOST_CHECK(c.getBool("mybool"));

  BOOST_CHECK_THROW(c.declareItem("mybrokenbool", Config::ConfigItemType::Bool, "some description", "some help", false, false, "foo"), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END();
