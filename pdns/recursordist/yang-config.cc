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

#include <libyang/Tree_Data.hpp>

#include "yang-config.hh"

namespace pdns
{
namespace config
{
  YangRecursorConfig::YangRecursorConfig(const std::string& yangdir, const std::string& configfile)
  {
    d_ctx = std::make_shared<libyang::Context>(yangdir.c_str(), LY_CTX_NOYANGLIBRARY | LY_CTX_DISABLE_SEARCHDIR_CWD);
    if (d_ctx == nullptr) {
      throw std::runtime_error("Unable to create libyang context");
    }
    d_ctx->load_module("pdns-recursor");
    d_config = std::move(d_ctx->parse_data_path(configfile.c_str(), LYD_JSON, LYD_OPT_CONFIG));
    if (d_config == nullptr) {
      throw std::runtime_error("Unable to parse file '" + configfile + "' to a libyang tree");
    }
  }

  struct dnssec_config YangRecursorConfig::dnssecConfig()
  {
    struct dnssec_config ret;
    ret.validation = getPathString("/pdns-recursor:dnssec/validation");
    ret.log_bogus = getPathBoolean("/pdns-recursor:dnssec/log-bogus");

    auto anchor_set = getPathSet("/pdns-recursor:dnssec/trust-anchor");
    for (auto const& anchor : anchor_set->data()) {
      DNSName domain;
      std::string ds_record;
      for (auto const& node : anchor->tree_dfs()) {
        std::string nodeName(node->schema()->name());
        if (nodeName == "domain") {
          auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
          domain = DNSName(leaf->value_str());
        }
        if (nodeName == "ds-record") {
          auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
          ds_record = leaf->value_str();
        }
      }
      auto ds = std::dynamic_pointer_cast<DSRecordContent>(DSRecordContent::make(ds_record));
      ret.trust_anchors.push_back({domain, ds});
    }

    auto negative_anchor_set = getPathSet("/pdns-recursor:dnssec/negative-trust-anchor");
    for (auto const& negative_anchor : negative_anchor_set->data()) {
      DNSName domain;
      std::string reason;
      for (auto const& node : negative_anchor->tree_dfs()) {
        std::string nodeName(node->schema()->name());
        if (nodeName == "domain") {
          auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
          domain = DNSName(leaf->value_str());
        }
        if (nodeName == "reason") {
          auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
          reason = leaf->value_str();
        }
      }
      ret.negative_trust_anchors.push_back({domain, reason});
    }
    return ret;
  }

  std::vector<listen_address> YangRecursorConfig::listenAddresses()
  {
    std::vector<listen_address> ret;
    auto vals = getPathSet("/pdns-recursor:listen-addresses/listen-address");
    for (auto const& val : vals->data()) {
      uint16_t port = 53;
      listen_address tmp;
      tmp.non_local_bind = false;
      for (auto const& node : val->tree_dfs()) {
        std::string nodeName(node->schema()->name());
        if (nodeName == "ip-address") {
          auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
          tmp.address = ComboAddress(leaf->value_str());
        }
        if (nodeName == "port") {
          auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
          port = leaf->value()->uint16();
        }
        if (nodeName == "non_local_bind") {
          auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
          tmp.non_local_bind = leaf->value()->bln();
        }
      }
      tmp.address.setPort(port);
      ret.push_back(tmp);
    }
    return ret;
  }

  libyang::S_Set YangRecursorConfig::getPathSet(const std::string& expr)
  {
    auto val_set = d_config->find_path(expr.c_str());
    return val_set;
  }

  libyang::S_Data_Node YangRecursorConfig::getPath(const std::string& expr)
  {
    auto val_set = d_config->find_path(expr.c_str());
    if (val_set->number() == 0) {
      return nullptr;
    }
    if (val_set->number() > 1) {
      throw std::runtime_error("More than one result for '" + expr + "'");
    }
    return val_set->data().at(0);
  }

  std::string YangRecursorConfig::getPathString(const std::string& expr)
  {
    auto node = getPath(expr);
    auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
    return leaf->value_str();
  }

  bool YangRecursorConfig::getPathBoolean(const std::string& expr)
  {
    auto node = getPath(expr);
    auto leaf = std::make_shared<libyang::Data_Node_Leaf_List>(node);
    if (leaf->value_type() != LY_TYPE_BOOL) {
      throw std::logic_error("Element at '" + expr + "' is not a bool");
    }
    return leaf->value()->bln();
  }
}
}
