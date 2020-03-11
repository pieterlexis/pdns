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

#include "yang-config.hh"

int main(int argc, char** argv) {
  if (argc != 3) {
    cerr<<"Please provide a path to the yang modules and a config file"<<endl;
    return 1;
  }
  libyang::set_log_verbosity(LY_LLDBG);
  try {
    auto cfg = pdns::config::YangRecursorConfig(std::string(argv[1]), std::string(argv[2]));
    auto dnssec_conf = cfg.dnssecConfig();
    cout<<"dnssec"<<endl;
    cout<<"  validation: "<<dnssec_conf.validation<<endl;
    cout<<"  log bogus: "<<(dnssec_conf.log_bogus ? "yes" : "no")<<endl;
    cout<<"  trust anchors: "<<endl;
    for (auto const &ta : dnssec_conf.trust_anchors) {
      cout<<"    "<<ta.first.toStringRootDot()<<" "<<ta.second->getZoneRepresentation()<<endl;
    }
    cout<<"  negative trust anchors: "<<endl;
    for (auto const &nta : dnssec_conf.negative_trust_anchors) {
      cout<<"    "<<nta.first.toStringRootDot()<<" '"<<nta.second<<"'"<<endl;
    }
    cout<<"listen addresses"<<endl;
    auto listen_addrs = cfg.listenAddresses();
    for (auto const &la : listen_addrs) {
      cout<<"  "<<la.address.toStringWithPort()<<endl;
    }
  } catch(const std::exception &e) {
    cerr<<e.what()<<endl;
    return 1;
  }
}
