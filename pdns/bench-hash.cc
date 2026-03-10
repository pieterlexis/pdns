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
// NOLINTBEGIN(unused-includes)
#include "dnsname.hh" // Required to expose dns_tolower in burtle.hh
// NOLINTEND
#include "burtle.hh"
#include <string>
#include <vector>

#define CATCH_CONFIG_NO_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/benchmark/catch_chronometer.hpp>

TEST_CASE("hash string", "[hash]")
{

  std::vector<std::string> inStrings{
    {""},
    {"the"},
    {"the quick brown"},
    {"the quick brown fox jumped"},
    {"the quick brown fox jumped over the"},
    {"the quick brown fox jumped over the lazy fox"},
  };

  for (auto const& inStr : inStrings) {
    std::string benchmarkNameSuffix("burtle " + std::to_string(inStr.length()) + " chars");

    auto ucInStr = reinterpret_cast<const unsigned char*>(inStr.c_str());
    BENCHMARK(benchmarkNameSuffix.c_str())
    {
      return burtle(ucInStr, inStr.length(), 0);
    };

    benchmarkNameSuffix = "burtleCI " + std::to_string(inStr.length()) + " chars";
    BENCHMARK(benchmarkNameSuffix.c_str())
    {
      return burtleCI(ucInStr, inStr.length(), 0);
    };
  }
}
