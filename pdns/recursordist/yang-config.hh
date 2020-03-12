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

#include "config-base.hh"
#include <libyang/Libyang.hpp>

namespace pdns
{
namespace config
{
  class YangRecursorConfig : public RecursorConfig
  {
  public:
    /**
      * @brief Creates a new Configuration for the recursor
      *
      * @param yangdir     Directory to the yang module
      * @param configfile  Path to a JSON with config
      */
    YangRecursorConfig(const std::string& yangdir, const std::string& configfile);

    /**
      * @brief Get the full DNSSEC config
      *
      * @return struct dnssec_config 
      */
    struct dnssec_config dnssecConfig() override;

    /**
      * @brief Get all listen addresses plus options
      * 
      * @return std::vector<listen_address> 
      */
    std::vector<listen_address> listenAddresses() override;

    /**
     * @brief Get a query local address
     * 
     * @param family  either AF_INET or AF_INET6
     * @param port    Optional port for the returned ComboAddress
     * @return ComboAddress 
     */
    ComboAddress getQueryLocalAddress(int family, uint16_t port) override;

  private:
    /**
      * @brief Get the Node at a certain path
      * 
      * Will throw if there is no node, or more than one node
      * 
      * @param expr   XPath expression for the node
      * @return libyang::S_Data_Node 
      * @throw std::runtime_error when 0 or more than one nodes match expr
      */
    libyang::S_Data_Node getPath(const std::string& expr);

    /**
      * @brief Get the string for the node at expr
      * 
      * @param expr   XPath expression for the node
      * @return std::string 
      * @throw std::runtime_error when 0 or more than one nodes match expr
      */
    std::string getPathString(const std::string& expr);

    /**
      * @brief Get the boolean for the node at expr
      * 
      * @param expr   XPath expression for the node
      * @return bool  Value of the node
      * @throw std::runtime_error when 0 or more than one nodes match expr
      */
    bool getPathBoolean(const std::string& expr);

    /**
      * @brief Get the set of nodes mathing expr
      * 
      * @param expr 
      * @return libyang::S_Set 
      */
    libyang::S_Set getPathSet(const std::string& expr);

    void setQLA();

    /**
      * @brief The libyang::Context for this instance
      */
    libyang::S_Context d_ctx;

    /**
      * @brief The root datanode
      */
    libyang::S_Data_Node d_config;

    /**
     * @brief Keep the QLA addresses on a per-family basis
     * 
     */
    std::vector<ComboAddress> d_query_local_addresses4;
    std::vector<ComboAddress> d_query_local_addresses6;

    /**
     * @brief Default QLA'a
     * 
     */
    static const ComboAddress s_query_local_address4;
    static const ComboAddress s_query_local_address6;
  };
}
}
