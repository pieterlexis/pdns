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

#include <cstddef>

#include "config.h"
#include "ext/luawrapper/include/LuaContext.hpp"
#include "rec-lua-conf.hh"

#include "namespaces.hh"
#include "lua-base4.hh"
#include "sortlist.hh"
#include "filterpo.hh"
#include "syncres.hh"
#include "rpzloader.hh"
#include "base64.hh"
#include "root-dnssec.hh"
#include "rec-system-resolve.hh"

GlobalStateHolder<LuaConfigItems> g_luaconfs;

/* SO HOW DOES THIS WORK! AND PLEASE PAY ATTENTION!
   This function can be called at any time. It is expected to overwrite all the contents
   of LuaConfigItems, which is held in a GlobalStateHolder for RCU properties.

   This function can be called again at a later date, so you must make sure that anything you
   allow to be configured from here lives in g_luaconfs AND NOWHERE ELSE.

   If someone loads an empty Lua file, the default LuaConfigItems struct MUST MAKE SENSE.

   To make this easy on you, here is a LuaConfigItems constructor where you
   can set sane defaults:
*/

LuaConfigItems::LuaConfigItems()
{
  DNSName root("."); // don't use g_rootdnsname here, it might not exist yet
  for (const auto& dsRecord : rootDSs) {
    auto dsRecContent = std::dynamic_pointer_cast<DSRecordContent>(DSRecordContent::make(dsRecord));
    dsAnchors[root].emplace(*dsRecContent);
  }
}

/* DID YOU READ THE STORY ABOVE? */

bool operator==(const ProtobufExportConfig& configA, const ProtobufExportConfig& configB)
{
  // clang-format off
  return configA.exportTypes          == configB.exportTypes       &&
         configA.servers              == configB.servers           &&
         configA.maxQueuedEntries     == configB.maxQueuedEntries  &&
         configA.timeout              == configB.timeout           &&
         configA.reconnectWaitTime    == configB.reconnectWaitTime &&
         configA.asyncConnect         == configB.asyncConnect      &&
         configA.enabled              == configB.enabled           &&
         configA.logQueries           == configB.logQueries        &&
         configA.logResponses         == configB.logResponses      &&
         configA.taggedOnly           == configB.taggedOnly        &&
         configA.logMappedFrom        == configB.logMappedFrom;
  // clang-format on
}

bool operator!=(const ProtobufExportConfig& configA, const ProtobufExportConfig& configB)
{
  return !(configA == configB);
}

bool operator==(const FrameStreamExportConfig& configA, const FrameStreamExportConfig& configB)
{
  // clang-format off
  return configA.enabled              == configB.enabled              &&
         configA.logQueries           == configB.logQueries           &&
         configA.logResponses         == configB.logResponses         &&
         configA.logNODs              == configB.logNODs              &&
         configA.logUDRs              == configB.logUDRs              &&
         configA.bufferHint           == configB.bufferHint           &&
         configA.flushTimeout         == configB.flushTimeout         &&
         configA.inputQueueSize       == configB.inputQueueSize       &&
         configA.outputQueueSize      == configB.outputQueueSize      &&
         configA.queueNotifyThreshold == configB.queueNotifyThreshold &&
         configA.reopenInterval       == configB.reopenInterval       &&
         configA.servers              == configB.servers;
  // clang-format on
}

bool operator!=(const FrameStreamExportConfig& configA, const FrameStreamExportConfig& configB)
{
  return !(configA == configB);
}

using rpzOptions_t = std::unordered_map<std::string, std::variant<bool, uint32_t, std::string, std::vector<std::pair<int, std::string>>>>;

static void parseRPZParameters(const rpzOptions_t& have, RPZTrackerParams& params)
{
  if (have.count("policyName") != 0) {
    params.polName = std::get<std::string>(have.at("policyName"));
  }
  if (have.count("defpol") != 0) {
    params.defpol = DNSFilterEngine::Policy();
    params.defpol->d_kind = (DNSFilterEngine::PolicyKind)std::get<uint32_t>(have.at("defpol"));
    params.defpol->setName(params.polName);
    if (params.defpol->d_kind == DNSFilterEngine::PolicyKind::Custom) {
      params.defcontent = std::get<string>(have.at("defcontent"));
      if (!params.defpol->d_custom) {
        params.defpol->d_custom = make_unique<DNSFilterEngine::Policy::CustomData>();
      }
      params.defpol->d_custom->push_back(DNSRecordContent::make(QType::CNAME, QClass::IN,
                                                                params.defcontent));

      if (have.count("defttl") != 0) {
        params.defpol->d_ttl = static_cast<int32_t>(std::get<uint32_t>(have.at("defttl")));
      }
      else {
        params.defpol->d_ttl = -1; // get it from the zone
      }
    }

    if (have.count("defpolOverrideLocalData") != 0) {
      params.defpolOverrideLocal = std::get<bool>(have.at("defpolOverrideLocalData"));
    }
  }
  if (have.count("maxTTL") != 0) {
    params.maxTTL = std::get<uint32_t>(have.at("maxTTL"));
  }
  if (have.count("zoneSizeHint") != 0) {
    params.zoneXFRParams.zoneSizeHint = static_cast<size_t>(std::get<uint32_t>(have.at("zoneSizeHint")));
  }
  if (have.count("tags") != 0) {
    const auto& tagsTable = std::get<std::vector<std::pair<int, std::string>>>(have.at("tags"));
    std::unordered_set<std::string> tags;
    for (const auto& tag : tagsTable) {
      tags.insert(tag.second);
      params.tags.insert(tag.second);
    }
  }
  if (have.count("overridesGettag") != 0) {
    params.defpolOverrideLocal = std::get<bool>(have.at("overridesGettag"));
  }
  if (have.count("extendedErrorCode") != 0) {
    auto code = std::get<uint32_t>(have.at("extendedErrorCode"));
    if (code > std::numeric_limits<uint16_t>::max()) {
      throw std::runtime_error("Invalid extendedErrorCode value " + std::to_string(code) + " in RPZ configuration");
    }
    params.extendedErrorCode = code;
    if (have.count("extendedErrorExtra") != 0) {
      params.extendedErrorExtra = std::get<std::string>(have.at("extendedErrorExtra"));
    }
  }
  if (have.count("includeSOA") != 0) {
    params.includeSOA = std::get<bool>(have.at("includeSOA"));
  }
  if (have.count("ignoreDuplicates") != 0) {
    params.ignoreDuplicates = std::get<bool>(have.at("ignoreDuplicates"));
  }
}

using protobufOptions_t = std::unordered_map<std::string, std::variant<bool, uint64_t, std::string, std::vector<std::pair<int, std::string>>>>;

static void parseProtobufOptions(const std::optional<protobufOptions_t>& vars, ProtobufExportConfig& config)
{
  if (!vars) {
    return;
  }
  const auto& have = *vars;

  if (have.count("timeout") != 0) {
    config.timeout = std::get<uint64_t>(have.at("timeout"));
  }

  if (have.count("maxQueuedEntries") != 0) {
    config.maxQueuedEntries = std::get<uint64_t>(have.at("maxQueuedEntries"));
  }

  if (have.count("reconnectWaitTime") != 0) {
    config.reconnectWaitTime = std::get<uint64_t>(have.at("reconnectWaitTime"));
  }

  if (have.count("asyncConnect") != 0) {
    config.asyncConnect = std::get<bool>(have.at("asyncConnect"));
  }

  if (have.count("taggedOnly") != 0) {
    config.taggedOnly = std::get<bool>(have.at("taggedOnly"));
  }

  if (have.count("logQueries") != 0) {
    config.logQueries = std::get<bool>(have.at("logQueries"));
  }

  if (have.count("logResponses") != 0) {
    config.logResponses = std::get<bool>(have.at("logResponses"));
  }

  if (have.count("logMappedFrom") != 0) {
    config.logMappedFrom = std::get<bool>(have.at("logMappedFrom"));
  }

  if (have.count("exportTypes") != 0) {
    config.exportTypes.clear();

    auto types = std::get<std::vector<std::pair<int, std::string>>>(have.at("exportTypes"));
    for (const auto& pair : types) {
      const auto& type = pair.second;

      QType qtype;
      try {
        qtype = std::stoul(type);
      }
      catch (const std::exception& ex) {
        qtype = QType::chartocode(type.c_str());
        if (qtype == 0) {
          throw std::runtime_error("Unknown QType '" + type + "' in protobuf's export types");
        }
      }
      config.exportTypes.insert(qtype);
    }
  }
}

#ifdef HAVE_FSTRM
using frameStreamOptions_t = std::unordered_map<std::string, std::variant<bool, uint64_t, std::string, std::vector<std::pair<int, std::string>>>>;

static void parseFrameStreamOptions(const std::optional<frameStreamOptions_t>& vars, FrameStreamExportConfig& config)
{
  if (!vars) {
    return;
  }
  const auto& have = *vars;

  if (have.count("logQueries") != 0) {
    config.logQueries = std::get<bool>(have.at("logQueries"));
  }
  if (have.count("logResponses") != 0) {
    config.logResponses = std::get<bool>(have.at("logResponses"));
  }
  if (have.count("logNODs") != 0) {
    config.logNODs = std::get<bool>(have.at("logNODs"));
  }
  if (have.count("logUDRs") != 0) {
    config.logUDRs = std::get<bool>(have.at("logUDRs"));
  }

  if (have.count("bufferHint") != 0) {
    config.bufferHint = std::get<uint64_t>(have.at("bufferHint"));
  }
  if (have.count("flushTimeout") != 0) {
    config.flushTimeout = std::get<uint64_t>(have.at("flushTimeout"));
  }
  if (have.count("inputQueueSize") != 0) {
    config.inputQueueSize = std::get<uint64_t>(have.at("inputQueueSize"));
  }
  if (have.count("outputQueueSize") != 0) {
    config.outputQueueSize = std::get<uint64_t>(have.at("outputQueueSize"));
  }
  if (have.count("queueNotifyThreshold") != 0) {
    config.queueNotifyThreshold = std::get<uint64_t>(have.at("queueNotifyThreshold"));
  }
  if (have.count("reopenInterval") != 0) {
    config.reopenInterval = std::get<uint64_t>(have.at("reopenInterval"));
  }
}
#endif /* HAVE_FSTRM */

static void rpzPrimary(LuaConfigItems& lci, const std::variant<string, std::vector<std::pair<int, string>>>& primaries_, const string& zoneName, const std::optional<rpzOptions_t>& options)
{
  RPZTrackerParams params;
  params.zoneXFRParams.name = zoneName;
  params.polName = zoneName;

  std::shared_ptr<DNSFilterEngine::Zone> zone = std::make_shared<DNSFilterEngine::Zone>();
  if (const auto& sval = std::get_if<std::string>(&primaries_)) {
    params.zoneXFRParams.primaries.emplace_back(*sval);
  }
  else {
    for (const auto& primary : std::get<std::vector<std::pair<int, std::string>>>(primaries_)) {
      params.zoneXFRParams.primaries.emplace_back(primary.second);
    }
  }

  try {
    if (options) {
      const auto& have = *options;
      parseRPZParameters(have, params);

      if (have.count("tsigname") != 0) {
        params.zoneXFRParams.tsigtriplet.name = DNSName(toLower(std::get<string>(have.at("tsigname"))));
        params.zoneXFRParams.tsigtriplet.algo = DNSName(toLower(std::get<string>(have.at("tsigalgo"))));
        if (B64Decode(std::get<string>(have.at("tsigsecret")), params.zoneXFRParams.tsigtriplet.secret) != 0) {
          throw std::runtime_error("TSIG secret is not valid Base-64 encoded");
        }
      }
      if (have.count("refresh") != 0) {
        params.zoneXFRParams.refreshFromConf = std::get<uint32_t>(have.at("refresh"));
        if (params.zoneXFRParams.refreshFromConf == 0) {
          lci.d_slog->info(Logr::Warning, "rpzPrimary refresh value of 0 ignored");
        }
      }

      if (have.count("maxReceivedMBytes") != 0) {
        params.zoneXFRParams.maxReceivedMBytes = static_cast<size_t>(std::get<uint32_t>(have.at("maxReceivedMBytes")));
      }

      if (have.count("localAddress") != 0) {
        params.zoneXFRParams.localAddress = ComboAddress(std::get<string>(have.at("localAddress")));
      }

      if (have.count("axfrTimeout") != 0) {
        params.zoneXFRParams.xfrTimeout = static_cast<uint16_t>(std::get<uint32_t>(have.at("axfrTimeout")));
      }

      if (have.count("seedFile") != 0) {
        params.seedFileName = std::get<std::string>(have.at("seedFile"));
      }

      if (have.count("dumpFile") != 0) {
        params.dumpZoneFileName = std::get<std::string>(have.at("dumpFile"));
      }
    }

    if (params.zoneXFRParams.localAddress != ComboAddress()) {
      // We were passed a localAddress, check if its AF matches the primaries'
      for (const auto& nameOrIP : params.zoneXFRParams.primaries) {
        auto primary = pdns::fromNameOrIP(nameOrIP, 53, lci.d_slog);
        if (params.zoneXFRParams.localAddress.sin4.sin_family != primary.sin4.sin_family) {
          throw PDNSException("Primary address(" + primary.toString() + ") is not of the same Address Family as the local address (" + params.zoneXFRParams.localAddress.toString() + ").");
        }
      }
    }
    lci.rpzs.emplace_back(params);
  }
  catch (const std::exception& e) {
    lci.d_slog->error(Logr::Error, e.what(), "Exception configuring 'rpzPrimary'", "exception", Logging::Loggable("std::exception"));
  }
  catch (const PDNSException& e) {
    lci.d_slog->error(Logr::Error, e.reason, "Exception configuring 'rpzPrimary'", Logging::Loggable("PDNSException"));
  }
}

// A wrapper class that loads the standard Lua defintions into the context, so that we can use things like pdns.A
class RecLuaConfigContext : public BaseLua4
{
public:
  RecLuaConfigContext() :
    BaseLua4("")
  {
    prepareContext();
  }
  void postPrepareContext() override
  {
    // clang-format off
    d_pd.emplace_back("AdditionalMode", in_t{
          {"Ignore", static_cast<int>(AdditionalMode::Ignore)},
          {"CacheOnly", static_cast<int>(AdditionalMode::CacheOnly)},
          {"CacheOnlyRequireAuth", static_cast<int>(AdditionalMode::CacheOnlyRequireAuth)},
          {"ResolveImmediately", static_cast<int>(AdditionalMode::ResolveImmediately)},
          {"ResolveDeferred", static_cast<int>(AdditionalMode::ResolveDeferred)}
        });
  }
  void postLoad() override
  {
  }
  LuaContext* operator->()
  {
    return d_lw.get();
  }
};

void loadRecursorLuaConfig(const std::string& fname, ProxyMapping& proxyMapping, LuaConfigItems& newLuaConfig) // NOLINT(readability-function-cognitive-complexity)
{
  LuaConfigItems lci;
  if (g_slog) {
    lci.d_slog = g_slog->withName("luaconfig");
  }

  RecLuaConfigContext Lua;

  if (fname.empty()) {
    return;
  }
  ifstream ifs(fname);
  if (!ifs) {
    throw PDNSException("Cannot open file '" + fname + "': " + stringerror());
  }

  auto luaconfsLocal = g_luaconfs.getLocal();
  lci.generation = luaconfsLocal->generation + 1;

  Lua->writeFunction("clearSortlist", [&lci]() { lci.sortlist.clear(); });

  /* we can get: "1.2.3.4"
                 {"1.2.3.4", "4.5.6.7"}
                 {"1.2.3.4", {"4.5.6.7", "8.9.10.11"}}
  */

  map<string, DNSFilterEngine::PolicyKind> pmap{
    {"NoAction", DNSFilterEngine::PolicyKind::NoAction},
    {"Drop", DNSFilterEngine::PolicyKind::Drop},
    {"NXDOMAIN", DNSFilterEngine::PolicyKind::NXDOMAIN},
    {"NODATA", DNSFilterEngine::PolicyKind::NODATA},
    {"Truncate", DNSFilterEngine::PolicyKind::Truncate},
    {"Custom", DNSFilterEngine::PolicyKind::Custom}};
  Lua->writeVariable("Policy", pmap);

  // NOLINTNEXTLINE(performance-unnecessary-value-param) Lua wrapper does not handle optional &
  Lua->writeFunction("rpzFile", [&lci](const string& filename, std::optional<rpzOptions_t> options) {
    RPZTrackerParams params;
    params.zoneXFRParams.name = filename;
    params.polName = "rpzFile";
    if (options) {
      parseRPZParameters(*options, params);
    }
    lci.rpzs.emplace_back(params);
  });

  // NOLINTNEXTLINE(performance-unnecessary-value-param) Lua wrapper does not handle optional &
  Lua->writeFunction("rpzPrimary", [&lci](const std::variant<string, std::vector<std::pair<int, string>>>& primaries_, const string& zoneName, std::optional<rpzOptions_t> options) {
    rpzPrimary(lci, primaries_, zoneName, options);
  });

  using zoneToCacheOptions_t = std::unordered_map<std::string, std::variant<uint32_t, std::string>>;

  // NOLINTNEXTLINE(performance-unnecessary-value-param) Lua wrapper does not handle optional &
  Lua->writeFunction("zoneToCache", [&lci](const string& zoneName, const string& method, const std::variant<string, std::vector<std::pair<int, string>>>& srcs, std::optional<zoneToCacheOptions_t> options) {
    try {
      RecZoneToCache::Config conf;
      DNSName validZoneName(zoneName);
      conf.d_zone = zoneName;
      const set<string> methods = {"axfr", "url", "file"};
      if (methods.count(method) == 0) {
        throw std::runtime_error("unknown method '" + method + "'");
      }
      conf.d_method = method;
      if (const auto& sval = std::get_if<std::string>(&srcs)) {
        conf.d_sources.push_back(*sval);
      }
      else {
        for (const auto& src : std::get<std::vector<std::pair<int, std::string>>>(srcs)) {
          conf.d_sources.push_back(src.second);
        }
      }
      if (conf.d_sources.size() == 0) {
        throw std::runtime_error("at least one source required");
      }
      if (options) {
        auto& have = *options;
        if (have.count("timeout")) {
          conf.d_timeout = std::get<uint32_t>(have.at("timeout"));
        }
        if (have.count("tsigname")) {
          conf.d_tt.name = DNSName(toLower(std::get<string>(have.at("tsigname"))));
          conf.d_tt.algo = DNSName(toLower(std::get<string>(have.at("tsigalgo"))));
          if (B64Decode(std::get<string>(have.at("tsigsecret")), conf.d_tt.secret)) {
            throw std::runtime_error("TSIG secret is not valid Base-64 encoded");
          }
        }
        if (have.count("maxReceivedMBytes")) {
          conf.d_maxReceivedBytes = static_cast<size_t>(std::get<uint32_t>(have.at("maxReceivedMBytes")));
          conf.d_maxReceivedBytes *= static_cast<size_t>(1024 * 1024);
        }
        if (have.count("localAddress")) {
          conf.d_local = ComboAddress(std::get<string>(have.at("localAddress")));
        }
        if (have.count("refreshPeriod")) {
          conf.d_refreshPeriod = std::get<uint32_t>(have.at("refreshPeriod"));
        }
        if (have.count("retryOnErrorPeriod")) {
          conf.d_retryOnError = std::get<uint32_t>(have.at("retryOnErrorPeriod"));
        }
        const map<string, pdns::ZoneMD::Config> nameToVal = {
          {"ignore", pdns::ZoneMD::Config::Ignore},
          {"validate", pdns::ZoneMD::Config::Validate},
          {"require", pdns::ZoneMD::Config::Require},
        };
        if (have.count("zonemd")) {
          string zonemdValidation = std::get<string>(have.at("zonemd"));
          auto iter = nameToVal.find(zonemdValidation);
          if (iter == nameToVal.end()) {
            throw std::runtime_error(zonemdValidation + " is not a valid value for `zonemd`");
          }
          conf.d_zonemd = iter->second;
        }
        if (have.count("dnssec")) {
          string dnssec = std::get<string>(have.at("dnssec"));
          auto iter = nameToVal.find(dnssec);
          if (iter == nameToVal.end()) {
            throw std::runtime_error(dnssec + " is not a valid value for `dnssec`");
          }
          conf.d_dnssec = iter->second;
        }
      }

      lci.ztcConfigs[validZoneName] = std::move(conf);
    }
    catch (const std::exception& e) {
      lci.d_slog->error(Logr::Error, e.what(), "Problem configuring zoneToCache", "zone", Logging::Loggable(zoneName),
                        "exception", Logging::Loggable("std::exception"));
    }
  });

  using argvec_t = vector<pair<int, std::variant<string, vector<pair<int, string>>>>>;
  Lua->writeFunction("addSortList",
                     [&lci](const std::string& formask_,
                            const std::variant<string, argvec_t>& masks,
                            std::optional<int> order_) {
                       try {
                         Netmask formask(formask_);
                         int order = order_ ? (*order_) : lci.sortlist.getMaxOrder(formask) + 1;
                         if (const auto &str = std::get_if<string>(&masks)) {
                           lci.sortlist.addEntry(formask, Netmask(*str), order);
                         }
                         else {
                           auto vec = std::get<argvec_t>(masks);
                           for (const auto& vecentry : vec) {
                             if (const auto& value = std::get_if<string>(&vecentry.second)) {
                               lci.sortlist.addEntry(formask, Netmask(*value), order);
                             }
                             else {
                               const auto& entries = std::get<vector<pair<int, string>>>(vecentry.second);
                               for (const auto& entry : entries) {
                                 lci.sortlist.addEntry(formask, Netmask(entry.second), order);
                               }
                             }
                             ++order;
                           }
                         }
                       }
                       catch (std::exception& e) {
                         lci.d_slog->error(Logr::Error, e.what(), "Error in addSortList", "exception",  Logging::Loggable("std::exception"));
                       }
                     });

  Lua->writeFunction("addTA", [&lci](const std::string& who, const std::string& what) {
    DNSName zone(who);
    auto dsRecordContent = std::dynamic_pointer_cast<DSRecordContent>(DSRecordContent::make(what));
    lci.dsAnchors[zone].insert(*dsRecordContent);
  });

  Lua->writeFunction("clearTA", [&lci](std::optional<string> who) {
    if (who) {
      lci.dsAnchors.erase(DNSName(*who));
    }
    else {
      lci.dsAnchors.clear();
    }
  });

  Lua->writeFunction("addNTA", [&lci](const std::string& who, std::optional<std::string> why) {
    if (why) {
      lci.negAnchors[DNSName(who)] = static_cast<string>(*why);
    }
    else {
      lci.negAnchors[DNSName(who)] = "";
    }
  });

  Lua->writeFunction("clearNTA", [&lci](std::optional<string> who) {
    if (who) {
      lci.negAnchors.erase(DNSName(*who));
    }
    else {
      lci.negAnchors.clear();
    }
  });

  Lua->writeFunction("readTrustAnchorsFromFile", [&lci](const std::string& fnamearg, std::optional<uint32_t> interval) {
    uint32_t realInterval = 24;
    if (interval) {
      realInterval = static_cast<uint32_t>(*interval);
    }
    lci.trustAnchorFileInfo.fname = fnamearg;
    lci.trustAnchorFileInfo.interval = realInterval;
  });

  Lua->writeFunction("setProtobufMasks", [&lci](const uint8_t maskV4, uint8_t maskV6) {
    lci.protobufMaskV4 = maskV4;
    lci.protobufMaskV6 = maskV6;
  });

  // NOLINTNEXTLINE(performance-unnecessary-value-param) Lua wrapper does not handle optional &
  Lua->writeFunction("protobufServer", [&lci](std::variant<const std::string, const std::unordered_map<int, std::string>> servers, std::optional<protobufOptions_t> vars) {
    if (!lci.protobufExportConfig.enabled) {

      lci.protobufExportConfig.enabled = true;

      try {
        if (const auto& server = std::get_if<const std::string>(&servers)) {
          lci.protobufExportConfig.servers.emplace_back(*server);
        }
        else {
          auto serversMap = std::get<const std::unordered_map<int, std::string>>(servers);
          for (const auto& serverPair : serversMap) {
            lci.protobufExportConfig.servers.emplace_back(serverPair.second);
          }
        }

        parseProtobufOptions(vars, lci.protobufExportConfig);
      }
      catch (std::exception& e) {
        lci.d_slog->error(Logr::Error, e.what(), "Exception while adding protobuf logger", "exception", Logging::Loggable("std::exception"));
      }
      catch (PDNSException& e) {
        lci.d_slog->error(Logr::Error, e.reason, "Exception while adding protobuf logger", "exception", Logging::Loggable("PDNSException"));
      }
    }
    else {
      lci.d_slog->info(Logr::Error, "Only one protobufServer() directive can be configured", "existing", Logging::Loggable(lci.protobufExportConfig.servers.at(0).toString()));
    }
  });

  // NOLINTNEXTLINE(performance-unnecessary-value-param) Lua wrapper does not handle optional &
  Lua->writeFunction("outgoingProtobufServer", [&lci](std::variant<const std::string, const std::unordered_map<int, std::string>> servers, std::optional<protobufOptions_t> vars) {
    if (!lci.outgoingProtobufExportConfig.enabled) {

      lci.outgoingProtobufExportConfig.enabled = true;

      try {
        if (const auto& server = std::get_if<const std::string>(&servers)) {
          lci.outgoingProtobufExportConfig.servers.emplace_back(*server);
        }
        else {
          auto serversMap = std::get<const std::unordered_map<int, std::string>>(servers);
          for (const auto& serverPair : serversMap) {
            lci.outgoingProtobufExportConfig.servers.emplace_back(serverPair.second);
          }
        }

        parseProtobufOptions(vars, lci.outgoingProtobufExportConfig);
      }
      catch (std::exception& e) {
        lci.d_slog->error(Logr::Error, "Exception while starting outgoing protobuf logger", "exception", Logging::Loggable("std::exception"));
      }
      catch (PDNSException& e) {
        lci.d_slog->error(Logr::Error, "Exception while starting outgoing protobuf logger", "exception", Logging::Loggable("PDNSException"));
      }
    }
    else {
      lci.d_slog->info(Logr::Error, "Only one outgoingProtobufServer() directive can be configured", "existing", Logging::Loggable(lci.outgoingProtobufExportConfig.servers.at(0).toString()));
    }
  });

#ifdef HAVE_FSTRM
  // NOLINTNEXTLINE(performance-unnecessary-value-param) Lua wrapper does not handle optional &
  Lua->writeFunction("dnstapFrameStreamServer", [&lci](std::variant<const std::string, const std::unordered_map<int, std::string>> servers, std::optional<frameStreamOptions_t> vars) {
    if (!lci.frameStreamExportConfig.enabled) {

      lci.frameStreamExportConfig.enabled = true;

      try {
        if (const auto& server = std::get_if<const std::string>(&servers)) {
          if (!boost::starts_with(*server, "/")) {
            ComboAddress parsecheck(*server);
          }
          lci.frameStreamExportConfig.servers.emplace_back(*server);
        }
        else {
          auto serversMap = std::get<const std::unordered_map<int, std::string>>(servers);
          for (const auto& serverPair : serversMap) {
            lci.frameStreamExportConfig.servers.emplace_back(serverPair.second);
          }
        }

        parseFrameStreamOptions(vars, lci.frameStreamExportConfig);
      }
      catch (std::exception& e) {
        lci.d_slog->error(Logr::Error, "Exception reading config for dnstap framestream logger", "exception", Logging::Loggable("std::exception"));
      }
      catch (PDNSException& e) {
        lci.d_slog->error(Logr::Error, "Exception reading config for dnstap framestream logger", "exception", Logging::Loggable("PDNSException"));
      }
    }
    else {
      lci.d_slog->info(Logr::Error,  "Only one dnstapFrameStreamServer() directive can be configured",  "existing", Logging::Loggable(lci.frameStreamExportConfig.servers.at(0)));
    }
  });
  // NOLINTNEXTLINE(performance-unnecessary-value-param) Lua wrapper does not handle optional &
  Lua->writeFunction("dnstapNODFrameStreamServer", [&lci](std::variant<const std::string, const std::unordered_map<int, std::string>> servers, std::optional<frameStreamOptions_t> vars) {
    if (!lci.nodFrameStreamExportConfig.enabled) {
      lci.nodFrameStreamExportConfig.enabled = true;

      try {
        if (const auto& server = std::get_if<const std::string>(&servers)) {
          if (!boost::starts_with(*server, "/")) {
            ComboAddress parsecheck(*server);
          }
          lci.nodFrameStreamExportConfig.servers.emplace_back(*server);
        }
        else {
          auto serversMap = std::get<const std::unordered_map<int, std::string>>(servers);
          for (const auto& serverPair : serversMap) {
            lci.nodFrameStreamExportConfig.servers.emplace_back(serverPair.second);
          }
        }

        parseFrameStreamOptions(vars, lci.nodFrameStreamExportConfig);
      }
      catch (std::exception& e) {
        lci.d_slog->error(Logr::Error, "Exception reading config for dnstap NOD framestream logger", "exception", Logging::Loggable("std::exception"));
      }
      catch (PDNSException& e) {
        lci.d_slog->error(Logr::Error, "Exception reading config for dnstap NOD framestream logger", "exception", Logging::Loggable("PDNSException"));
      }
    }
    else {
      lci.d_slog->info(Logr::Error,  "Only one dnstapNODFrameStreamServer() directive can be configured",  "existing", Logging::Loggable(lci.nodFrameStreamExportConfig.servers.at(0)));
    }
  });
#endif /* HAVE_FSTRM */

  // NOLINTNEXTLINE(performance-unnecessary-value-param) Lua wrapper does not handle optional &
  Lua->writeFunction("addAllowedAdditionalQType", [&lci](int qtype, const std::unordered_map<int, int>& targetqtypes, std::optional<std::map<std::string, int>> options) {
    switch (qtype) {
    case QType::MX:
    case QType::SRV:
    case QType::SVCB:
    case QType::HTTPS:
    case QType::NAPTR:
      break;
    default:
      lci.d_slog->info(Logr::Error, "addAllowedAdditionalQType does not support this qtype", "qtype", Logging::Loggable(QType(qtype).toString()));
      return;
    }

    std::set<QType> targets;
    for (const auto& target : targetqtypes) {
      targets.emplace(target.second);
    }

    AdditionalMode mode = AdditionalMode::CacheOnlyRequireAuth; // Always cheap and should be safe

    if (options) {
      if (const auto iter = options->find("mode"); iter != options->end()) {
        mode = static_cast<AdditionalMode>(iter->second);
        if (mode > AdditionalMode::ResolveDeferred) {
          lci.d_slog->info(Logr::Error, "addAllowedAdditionalQType: unknown mode", "mode", Logging::Loggable( iter->second));
        }
      }
    }
    lci.allowAdditionalQTypes.insert_or_assign(qtype, pair(targets, mode));
  });

   // NOLINTNEXTLINE(performance-unnecessary-value-param) Lua wrapper does not handle optional &
 Lua->writeFunction("addProxyMapping", [&proxyMapping,&lci](const string& netmaskArg, const string& addressArg, std::optional<std::vector<pair<int,std::string>>> smnStrings) {
    try {
      Netmask netmask(netmaskArg);
      ComboAddress address(addressArg);
      std::optional<SuffixMatchNode> smn;
      if (smnStrings) {
        smn = std::make_optional(SuffixMatchNode{});
        for (const auto& subnet : *smnStrings) {
          smn->add(subnet.second);
        }
      }
      proxyMapping.insert_or_assign(netmask, {std::move(address), std::move(smn)});
    }
    catch (std::exception& e) {
      lci.d_slog->error(Logr::Error, e.what(), "Exception processing addProxyMapping", "exception", Logging::Loggable("std::exception"));
    }
    catch (PDNSException& e) {
      lci.d_slog->error(Logr::Error, e.reason, "Exception processing addProxyMapping", "exception", Logging::Loggable("PDNSException"));
    }
  });

  try {
    Lua->executeCode(ifs);
    newLuaConfig = std::move(lci);
  }
  catch (const LuaContext::ExecutionErrorException& e) {
    lci.d_slog->error(Logr::Error, e.what(),  "Unable to load Lua script", "file", Logging::Loggable(fname));
    try {
      std::rethrow_if_nested(e);
    }
    catch (const std::exception& exp) {
      // exp is the exception that was thrown from inside the lambda
      lci.d_slog->error(Logr::Error, exp.what(), "Exception loading Lua", "exception", Logging::Loggable("std::exception"));
    }
    catch (const PDNSException& exp) {
      // exp is the exception that was thrown from inside the lambda
      lci.d_slog->error(Logr::Error, exp.reason, "Exception loading Lua", "exception", Logging::Loggable("PDNSException"));
    }
    throw;
  }
  catch (std::exception& err) {
    lci.d_slog->error(Logr::Error, err.what(),  "Unable to load Lua script", "file", Logging::Loggable(fname), "exception", Logging::Loggable("std::exception"));
    throw;
  }
}
