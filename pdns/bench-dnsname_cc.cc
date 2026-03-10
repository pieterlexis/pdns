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
#include <vector>
#include "dnsname.hh"

#define CATCH_CONFIG_NO_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/benchmark/catch_chronometer.hpp>

std::vector<std::string> inNames{
  {""},
  {"."},
  {"com"},
  {"powerdns.com"},
  {"www.powerdns.com"},
  {"server15.www.powerdns.com"},
  {"foobar.server15.www.powerdns.com"},
};

TEST_CASE("DNSName::DNSName", "[dnsname,DNSName]")
{
  for (auto const& inStr : inNames) {
    std::string benchmarkNameSuffix("\"" + inStr + "\"");

    BENCHMARK(benchmarkNameSuffix.c_str())
    {
      return DNSName(inStr);
    };
  }
}


TEST_CASE("DNSName::chopoff", "[dnsname,DNSName]")
{
  for (auto const& inStr : inNames) {
    std::string benchmarkNameSuffix("\"" + inStr + "\"");

    BENCHMARK_ADVANCED(benchmarkNameSuffix.c_str())(Catch::Benchmark::Chronometer meter)
    {
      DNSName in(inStr);
      std::vector<DNSName> v(meter.runs());
      std::fill(v.begin(), v.end(), in);

      meter.measure([&v](int i){
        while(v[i].chopOff()) {}
        return v[i];
      });
    };
  }
}

TEST_CASE("DNSName:: label append/prepend", "[dnsname,DNSName]")
{
  std::string lbl{"example"};
  for (auto const& inStr : inNames) {
    DNSName in(inStr);

    std::string benchmarkNameSuffix("appendRawLabel to \"" + inStr + "\"");

    BENCHMARK_ADVANCED(benchmarkNameSuffix.c_str())(Catch::Benchmark::Chronometer meter)
    {
      std::vector<DNSName> v(meter.runs());
      std::fill(v.begin(), v.end(), in);
      meter.measure([&v, lbl](int i){
        return v[i].appendRawLabel(lbl);
      });
    };

    benchmarkNameSuffix = "prependRawLabel to \"" + inStr + "\"";
    BENCHMARK_ADVANCED(benchmarkNameSuffix.c_str())(Catch::Benchmark::Chronometer meter)
    {
      std::vector<DNSName> v(meter.runs());
      std::fill(v.begin(), v.end(), in);
      meter.measure([&v, lbl](int i){
        v[i].prependRawLabel(lbl);
      });
    };
  }
}
