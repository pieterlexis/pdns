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
#pragma once
#include <yaml-cpp/yaml.h>
#include "iputils.hh"
#include "pdnsexception.hh"

// Allows reading/writing Netmasks and ComboAddresses in YAML-cpp
namespace YAML {
  template<>
  struct convert<ComboAddress> {
    static Node encode(const ComboAddress& rhs) {
      return Node(rhs.toStringWithPort());
    }
    static bool decode(const Node& node, ComboAddress& rhs) {
      if (!node.IsScalar()) {
        return false;
      }
      try {
        rhs = ComboAddress(node.as<string>(), 53);
        return true;
      } catch(const runtime_error &e) {
        return false;
      } catch (const PDNSException &e) {
        return false;
      }
    }
  };


  template<>
  struct convert<Netmask> {
    static Node encode(const Netmask& rhs) {
      return Node(rhs.toString());
    }
    static bool decode(const Node& node, Netmask& rhs) {
      if (!node.IsScalar()) {
        return false;
      }
      try {
        rhs = Netmask(node.as<string>());
        return true;
      } catch(const runtime_error &e) {
        return false;
      } catch (const PDNSException &e) {
        return false;
      }
    }
  };
}
