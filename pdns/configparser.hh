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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <yaml-cpp/yaml.h>

#include "dnsname.hh"
#include "iputils.hh"

// Allows reading/writing ComboAddresses and DNSNames in YAML-cpp
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
struct convert<DNSName> {
  static Node encode(const DNSName& rhs) {
    return Node(rhs.toStringRootDot());
  }
  static bool decode(const Node& node, DNSName& rhs) {
    if (!node.IsScalar()) {
      return false;
    }
    try {
      rhs = DNSName(node.as<string>());
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
} // namespace YAML

template<typename T> class ConfigItem {
  public:
    explicit ConfigItem(const std::string &name, const std::string &description, const std::string &help, const bool runtime, const T &default_value) : d_name(name), d_description(description), d_help(help), d_runtime(runtime), d_default(default_value), d_node(default_value) {};

    void setValue(const T &val) {
      d_node = val;
    }

    T getValue() {
      return d_node.as<T>();
    }

    std::string getFullItem() {
      YAML::Emitter out;
      out << "# " << d_name << " " << d_description << '\n';
      out << "# " << d_help << '\n';
      out << d_node;
      return out.c_str();
    }

  private:
    std::string d_name;
    std::string d_description;
    std::string d_help;
    bool d_runtime;
    T d_default;

    YAML::Node d_node;
};

class Config {
  public:
    Config();
    ~Config();

    // enum class ConfigItemType { Bool, DNSName, IPEndpoint, IPNetmask, Number };

    template<typename T>
    void declareItem(const std::string &name, const std::string &description, const std::string &help, const bool runtime, const T &default_value);

    void parseConfig();

    template<typename T>
    T getConfigItem(const std::string& name);

  private:
    map<std::string, 
    bool d_configParsed{false};
    YAML::Node d_config;
};
