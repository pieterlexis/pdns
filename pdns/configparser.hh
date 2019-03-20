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

class ConfigItem {
  public:
    bool mandatory{true};
    bool runtime{false};
    std::string description;
    std::string help;

    virtual std::string getString() const;
    virtual std::string getDefaultString() const;
};

class ConfigBool : ConfigItem {
  public:
    bool getBool() {
      return d_v;
    }

    bool getDefaultBool() {
      return d_default;
    }

    std::string getString() const {
      if (d_v) {
        return "true";
      }
      return "false";
    }

    std::string getDefaultString() const {
      if (d_default) {
        return "true";
      }
      return "false";
    }

  private:
    bool d_v;
    bool d_default;
};

class ConfigIPEndpoint : ConfigItem {
  public:
    ComboAddress getIPEndpoint() const {
      return d_v;
    }
    ComboAddress getDefaultIPEndpoint() const {
      return d_default;
    }
    std::string getString() const {
      return d_v.toStringWithPort();
    }
    std::string getDefaultString() const {
      return d_default.toStringWithPort();
    }
  private:
    ComboAddress d_v;
    ComboAddress d_default;
};

class ConfigIPEndpoints : ConfigItem {
  public:
    vector<ComboAddress> getIPEndpoints() const {
      return d_v;
    }
    vector<ComboAddress> getDefaultIPEndpoint() const {
      return d_default;
    }

    std::string getString() const {
      string ret;
      bool first = true;
      for (auto const c : d_v) {
        if (!first) {
          ret += ", ";
        }
        first = false;
        ret += c.toStringWithPort();
      }
      return ret;
    }

    std::string getDefaultString() const {
      string ret;
      bool first = true;
      for (auto const c : d_default) {
        if (!first) {
          ret += ", ";
        }
        first = false;
        ret += c.toStringWithPort();
      }
      return ret;
    }

  private:
    vector<ComboAddress> d_v;
    vector<ComboAddress> d_default;
};

class Config {
  public:
    typedef struct {
      DNSName domain;
      std::vector<ComboAddress> masters;
    } SlaveDomain;

    enum class ConfigItemType {
      Bool,                 // bool
      DNSName,              // DNSName
      IPEndpoint,           // ComboAddress
      IPNetmask,            // Netmask
      Number,               // uint32_t
      SlaveDomain,          // struct SlaveDomain, used in ixfrdist
    };

    void declareItem(const std::string &name, const ConfigItemType valType, const std::string &description, const std::string &help, const bool runtime, const bool isVector, const boost::any defaultValue);
    void setValue(const std::string &name, const boost::any val);

    std::string getHelp(const std::string& name);
    std::string getDescription(const std::string& name);

    bool getBool(const std::string& name);

    DNSName getDNSName(const std::string& name);
    vector<DNSName> getDNSNames(const std::string& name);

    ComboAddress getIPEndpoint(const std::string& name);
    vector<ComboAddress> getIPEndpoints(const std::string& name);

    Netmask getIPNetmask(const std::string& name);
    vector<Netmask> getIPNetmasks(const std::string& name);

    uint32_t getNumber(const std::string& name);
    vector<uint32_t> getNumbers(const std::string& name);

    SlaveDomain getSlaveDomain(const std::string& name);
    vector<SlaveDomain> getSlaveDomains(const std::string& name);

    void parseConfig(const std::string &fname);
    string emitConfig();

  private:
    typedef struct {
      boost::any val;
      boost::any defaultValue;
      ConfigItemType t;
      bool isList;
      bool runtime;
      string description;
      string help;
    } configI;

    map<string, configI> configElements;
    bool d_configParsed{false};

    void isDeclared(const std::string &name) {
      if (configElements.count(name) != 1) {
        throw std::runtime_error("No configuration item with the name " + name + " has been declared");
      }
    }

    void tryCastVector(const boost::any &what, const ConfigItemType t) {
      if (what.empty()) { // No value is safe
        return;
      }
      try {
        switch(t) {
          case ConfigItemType::Bool:
            boost::any_cast<vector<bool>>(what);
            break;
          case ConfigItemType::DNSName:
            boost::any_cast<vector<DNSName>>(what);
            break;
          case ConfigItemType::IPEndpoint:
            boost::any_cast<vector<ComboAddress>>(what);
            break;
          case ConfigItemType::IPNetmask:
            boost::any_cast<vector<Netmask>>(what);
            break;
          case ConfigItemType::Number:
            boost::any_cast<vector<uint32_t>>(what);
            break;
          case ConfigItemType::SlaveDomain:
            boost::any_cast<vector<SlaveDomain>>(what);
            break;
        }
      } catch (const boost::bad_any_cast &e) {
        throw std::runtime_error("TODO");
      }
    }

    void tryCast(const boost::any &what, const ConfigItemType t) {
      if (what.empty()) { // No value is safe
        return;
      }
      try {
        switch(t) {
          case ConfigItemType::Bool:
            boost::any_cast<bool>(what);
            break;
          case ConfigItemType::DNSName:
            boost::any_cast<DNSName>(what);
            break;
          case ConfigItemType::IPEndpoint:
            boost::any_cast<ComboAddress>(what);
            break;
          case ConfigItemType::IPNetmask:
            boost::any_cast<Netmask>(what);
            break;
          case ConfigItemType::Number:
            boost::any_cast<uint32_t>(what);
            break;
          case ConfigItemType::SlaveDomain:
            boost::any_cast<SlaveDomain>(what);
            break;
        }
      } catch (const boost::bad_any_cast &e) {
        throw std::runtime_error("TODO");
      }
    }
};

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
  template<>
    struct convert<Config::SlaveDomain> {
      static Node encode(const Config::SlaveDomain& rhs) {
        Node node;
        node["domain"] = rhs.domain;
        node["masters"] = rhs.masters;
        return node;
      }

      static bool decode(const Node& node, Config::SlaveDomain& rhs) {
        if (!node["domain"]) {
          return false;
        }
        rhs.domain = node["domain"].as<DNSName>();
        if (!node["masters"]) {
          return false;
        }
        if (!node["masters"].IsSequence()){
          return false;
        }
        rhs.masters = node["masters"].as<std::vector<ComboAddress>>();
        return true;
      }
    };
} // namespace YAML
