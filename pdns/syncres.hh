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
#include <string>
#include <atomic>
#include "utility.hh"
#include "dns.hh"
#include "qtype.hh"
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <cmath>
#include <iostream>
#include <utility>
#include "misc.hh"
#include "lwres.hh"
#include <boost/optional.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/utility.hpp>
#include "sstuff.hh"
#include "recursor_cache.hh"
#include "recpacketcache.hh"
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include "mtasker.hh"
#include "iputils.hh"
#include "validate.hh"
#include "ednssubnet.hh"
#include "filterpo.hh"
#include "negcache.hh"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_PROTOBUF
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#endif

class RecursorLua4;

typedef map<
  DNSName,
  pair<
    vector<ComboAddress>,
    bool
  >
> NsSet;

template<class Thing> class Throttle : public boost::noncopyable
{
public:
  Throttle()
  {
    d_limit=3;
    d_ttl=60;
    d_last_clean=time(0);
  }
  bool shouldThrottle(time_t now, const Thing& t)
  {
    if(now > d_last_clean + 300 ) {

      d_last_clean=now;
      for(typename cont_t::iterator i=d_cont.begin();i!=d_cont.end();) {
        if( i->second.ttd < now) {
          d_cont.erase(i++);
        }
        else
          ++i;
      }
    }

    typename cont_t::iterator i=d_cont.find(t);
    if(i==d_cont.end())
      return false;
    if(now > i->second.ttd || i->second.count == 0) {
      d_cont.erase(i);
      return false;
    }
    i->second.count--;

    return true; // still listed, still blocked
  }
  void throttle(time_t now, const Thing& t, time_t ttl=0, unsigned int tries=0)
  {
    typename cont_t::iterator i=d_cont.find(t);
    entry e={ now+(ttl ? ttl : d_ttl), tries ? tries : d_limit};

    if(i==d_cont.end()) {
      d_cont[t]=e;
    }
    else if(i->second.ttd > e.ttd || (i->second.count) < e.count)
      d_cont[t]=e;
  }

  unsigned int size() const
  {
    return (unsigned int)d_cont.size();
  }

  void clear()
  {
    d_cont.clear();
  }
private:
  unsigned int d_limit;
  time_t d_ttl;
  time_t d_last_clean;
  struct entry
  {
    time_t ttd;
    unsigned int count;
  };
  typedef map<Thing,entry> cont_t;
  cont_t d_cont;
};


/** Class that implements a decaying EWMA.
    This class keeps an exponentially weighted moving average which, additionally, decays over time.
    The decaying is only done on get.
*/
class DecayingEwma
{
public:
  DecayingEwma() :  d_val(0.0)
  {
    d_needinit=true;
    d_last.tv_sec = d_last.tv_usec = 0;
    d_lastget=d_last;
  }

  DecayingEwma(const DecayingEwma& orig) : d_last(orig.d_last),  d_lastget(orig.d_lastget), d_val(orig.d_val), d_needinit(orig.d_needinit)
  {
  }

  void submit(int val, const struct timeval* tv)
  {
    struct timeval now=*tv;

    if(d_needinit) {
      d_last=now;
      d_lastget=now;
      d_needinit=false;
      d_val = val;
    }
    else {
      float diff= makeFloat(d_last - now);

      d_last=now;
      double factor=exp(diff)/2.0; // might be '0.5', or 0.0001
      d_val=(float)((1-factor)*val+ (float)factor*d_val);
    }
  }

  double get(const struct timeval* tv)
  {
    struct timeval now=*tv;
    float diff=makeFloat(d_lastget-now);
    d_lastget=now;
    float factor=exp(diff/60.0f); // is 1.0 or less
    return d_val*=factor;
  }

  double peek(void) const
  {
    return d_val;
  }

  bool stale(time_t limit) const
  {
    return limit > d_lastget.tv_sec;
  }

private:
  struct timeval d_last;          // stores time
  struct timeval d_lastget;       // stores time
  float d_val;
  bool d_needinit;
};

template<class Thing> class Counters : public boost::noncopyable
{
public:
  Counters()
  {
  }
  unsigned long value(const Thing& t) const
  {
    typename cont_t::const_iterator i=d_cont.find(t);

    if(i==d_cont.end()) {
      return 0;
    }
    return (unsigned long)i->second;
  }
  unsigned long incr(const Thing& t)
  {
    typename cont_t::iterator i=d_cont.find(t);

    if(i==d_cont.end()) {
      d_cont[t]=1;
      return 1;
    }
    else {
      if (i->second < std::numeric_limits<unsigned long>::max())
        i->second++;
      return (unsigned long)i->second;
   }
  }
  unsigned long decr(const Thing& t)
  {
    typename cont_t::iterator i=d_cont.find(t);

    if(i!=d_cont.end() && --i->second == 0) {
      d_cont.erase(i);
      return 0;
    } else
      return (unsigned long)i->second;
  }
  void clear(const Thing& t)
  {
    typename cont_t::iterator i=d_cont.find(t);

    if(i!=d_cont.end()) {
      d_cont.erase(i);
    }
  }
  void clear()
  {
    d_cont.clear();
  }
  size_t size() const
  {
    return d_cont.size();
  }
private:
  typedef map<Thing,unsigned long> cont_t;
  cont_t d_cont;
};


class SyncRes : public boost::noncopyable
{
public:
  enum LogMode { LogNone, Log, Store};
  typedef std::function<int(const ComboAddress& ip, const DNSName& qdomain, int qtype, bool doTCP, bool sendRDQuery, int EDNS0Level, struct timeval* now, boost::optional<Netmask>& srcmask, boost::optional<const ResolveContext&> context, std::shared_ptr<RemoteLogger> outgoingLogger, LWResult *lwr)> asyncresolve_t;

  struct EDNSStatus
  {
    EDNSStatus() : mode(UNKNOWN), modeSetAt(0) {}
    enum EDNSMode { UNKNOWN=0, EDNSOK=1, EDNSIGNORANT=2, NOEDNS=3 } mode;
    time_t modeSetAt;
  };

    //! This represents a number of decaying Ewmas, used to store performance per nameserver-name.
  /** Modelled to work mostly like the underlying DecayingEwma. After you've called get,
      d_best is filled out with the best address for this collection */
  struct DecayingEwmaCollection
  {
    void submit(const ComboAddress& remote, int usecs, const struct timeval* now)
    {
      collection_t::iterator pos;
      for(pos=d_collection.begin(); pos != d_collection.end(); ++pos)
        if(pos->first==remote)
          break;
      if(pos!=d_collection.end()) {
        pos->second.submit(usecs, now);
      }
      else {
        DecayingEwma de;
        de.submit(usecs, now);
        d_collection.push_back(make_pair(remote, de));
      }
    }

    double get(const struct timeval* now)
    {
      if(d_collection.empty())
        return 0;
      double ret=std::numeric_limits<double>::max();
      double tmp;
      for(collection_t::iterator pos=d_collection.begin(); pos != d_collection.end(); ++pos) {
        if((tmp=pos->second.get(now)) < ret) {
          ret=tmp;
          d_best=pos->first;
        }
      }

      return ret;
    }

    bool stale(time_t limit) const
    {
      for(collection_t::const_iterator pos=d_collection.begin(); pos != d_collection.end(); ++pos)
        if(!pos->second.stale(limit))
          return false;
      return true;
    }

    typedef vector<pair<ComboAddress, DecayingEwma> > collection_t;
    collection_t d_collection;
    ComboAddress d_best;
  };

  typedef map<DNSName, DecayingEwmaCollection> nsspeeds_t;
  typedef map<ComboAddress, EDNSStatus> ednsstatus_t;

  class AuthDomain
  {
  public:
    typedef multi_index_container <
      DNSRecord,
      indexed_by <
        ordered_non_unique<
          composite_key< DNSRecord,
                         member<DNSRecord, DNSName, &DNSRecord::d_name>,
                         member<DNSRecord, uint16_t, &DNSRecord::d_type>
                       >,
          composite_key_compare<std::less<DNSName>, std::less<uint16_t> >
        >
      >
    > records_t;

    records_t d_records;
    vector<ComboAddress> d_servers;
    DNSName d_name;
    bool d_rdForward{false};

    int getRecords(const DNSName& qname, uint16_t qtype, std::vector<DNSRecord>& records) const;
    bool isAuth() const
    {
      return d_servers.empty();
    }
    bool isForward() const
    {
      return !isAuth();
    }
    bool shouldRecurse() const
    {
      return d_rdForward;
    }
    const DNSName& getName() const
    {
      return d_name;
    }

  private:
    void addSOA(std::vector<DNSRecord>& records) const;
  };

  typedef map<DNSName, AuthDomain> domainmap_t;
  typedef Throttle<boost::tuple<ComboAddress,DNSName,uint16_t> > throttle_t;
  typedef Counters<ComboAddress> fails_t;

  struct ThreadLocalStorage {
    NegCache negcache;
    nsspeeds_t nsSpeeds;
    throttle_t throttle;
    ednsstatus_t ednsstatus;
    fails_t fails;
    std::shared_ptr<domainmap_t> domainmap;
  };

  static void setDefaultLogMode(LogMode lm)
  {
    s_lm = lm;
  }
  static void doEDNSDumpAndClose(int fd);
  static uint64_t doDumpNSSpeeds(int fd);
  static int getRootNS(struct timeval now, asyncresolve_t asyncCallback);
  static void clearDelegationOnly()
  {
    s_delegationOnly.clear();
  }
  static void addDelegationOnly(const DNSName& name)
  {
    s_delegationOnly.insert(name);
  }
  static void addDontQuery(const std::string& mask)
  {
    if (!s_dontQuery)
      s_dontQuery = std::unique_ptr<NetmaskGroup>(new NetmaskGroup());

    s_dontQuery->addMask(mask);
  }
  static void addDontQuery(const Netmask& mask)
  {
    if (!s_dontQuery)
      s_dontQuery = std::unique_ptr<NetmaskGroup>(new NetmaskGroup());

    s_dontQuery->addMask(mask);
  }
  static void clearDontQuery()
  {
    s_dontQuery = nullptr;
  }
  static void parseEDNSSubnetWhitelist(const std::string& wlist);
  static void addEDNSSubnet(const Netmask& subnet)
  {
    s_ednssubnets.addMask(subnet);
  }
  static void addEDNSDomain(const DNSName& domain)
  {
    s_ednsdomains.add(domain);
  }
  static void clearEDNSSubnets()
  {
    s_ednssubnets.clear();
  }
  static void clearEDNSDomains()
  {
    s_ednsdomains = SuffixMatchNode();
  }
  static void pruneNSSpeeds(time_t limit)
  {
    for(auto i = t_sstorage.nsSpeeds.begin(), end = t_sstorage.nsSpeeds.end(); i != end; ) {
      if(i->second.stale(limit)) {
        i = t_sstorage.nsSpeeds.erase(i);
      }
      else {
        ++i;
      }
    }
  }
  static uint64_t getNSSpeedsSize()
  {
    return t_sstorage.nsSpeeds.size();
  }
  static void submitNSSpeed(const DNSName& server, const ComboAddress& ca, uint32_t usec, const struct timeval* now)
  {
    t_sstorage.nsSpeeds[server].submit(ca, usec, now);
  }
  static void clearNSSpeeds()
  {
    t_sstorage.nsSpeeds.clear();
  }
  static EDNSStatus::EDNSMode getEDNSStatus(const ComboAddress& server)
  {
    const auto& it = t_sstorage.ednsstatus.find(server);
    if (it == t_sstorage.ednsstatus.end())
      return EDNSStatus::UNKNOWN;

    return it->second.mode;
  }
  static uint64_t getEDNSStatusesSize()
  {
    return t_sstorage.ednsstatus.size();
  }
  static void clearEDNSStatuses()
  {
    t_sstorage.ednsstatus.clear();
  }
  static uint64_t getThrottledServersSize()
  {
    return t_sstorage.throttle.size();
  }
  static void clearThrottle()
  {
    t_sstorage.throttle.clear();
  }
  static bool isThrottled(time_t now, const ComboAddress& server, const DNSName& target, uint16_t qtype)
  {
    return t_sstorage.throttle.shouldThrottle(now, boost::make_tuple(server, target, qtype));
  }
  static bool isThrottled(time_t now, const ComboAddress& server)
  {
    return t_sstorage.throttle.shouldThrottle(now, boost::make_tuple(server, "", 0));
  }
  static void doThrottle(time_t now, const ComboAddress& server, time_t duration, unsigned int tries)
  {
    t_sstorage.throttle.throttle(now, boost::make_tuple(server, "", 0), duration, tries);
  }
  static uint64_t getFailedServersSize()
  {
    return t_sstorage.fails.size();
  }
  static void clearFailedServers()
  {
    t_sstorage.fails.clear();
  }
  static unsigned long getServerFailsCount(const ComboAddress& server)
  {
    return t_sstorage.fails.value(server);
  }

  static void clearNegCache()
  {
    t_sstorage.negcache.clear();
  }

  static uint64_t getNegCacheSize()
  {
    return t_sstorage.negcache.size();
  }

  static void pruneNegCache(unsigned int maxEntries)
  {
    t_sstorage.negcache.prune(maxEntries);
  }

  static uint64_t wipeNegCache(const DNSName& name, bool subtree = false)
  {
    return t_sstorage.negcache.wipe(name, subtree);
  }

  static void setDomainMap(std::shared_ptr<domainmap_t> newMap)
  {
    t_sstorage.domainmap = newMap;
  }

  static const std::shared_ptr<domainmap_t> getDomainMap()
  {
    return t_sstorage.domainmap;
  }

  explicit SyncRes(const struct timeval& now);

  int beginResolve(const DNSName &qname, const QType &qtype, uint16_t qclass, vector<DNSRecord>&ret);
  void setId(int id)
  {
    if(doLog())
      d_prefix="["+itoa(id)+"] ";
  }

  void setLogMode(LogMode lm)
  {
    d_lm = lm;
  }

  bool doLog() const
  {
    return d_lm != LogNone;
  }

  void setCacheOnly(bool state=true)
  {
    d_cacheonly=state;
  }

  void setDoEDNS0(bool state=true)
  {
    d_doEDNS0=state;
  }

  void setDoDNSSEC(bool state=true)
  {
    d_doDNSSEC=state;
  }

  void setWantsRPZ(bool state=true)
  {
    d_wantsRPZ=state;
  }

  bool getWantsRPZ() const
  {
    return d_wantsRPZ;
  }

  void setIncomingECSFound(bool state=true)
  {
    d_incomingECSFound=state;
  }

  string getTrace() const
  {
    return d_trace.str();
  }

  void setLuaEngine(shared_ptr<RecursorLua4> pdl)
  {
    d_pdl = pdl;
  }

  bool wasVariable() const
  {
    return d_wasVariable;
  }

  bool wasOutOfBand() const
  {
    return d_wasOutOfBand;
  }

  struct timeval getNow() const
  {
    return d_now;
  }

  void setSkipCNAMECheck(bool skip = false)
  {
    d_skipCNAMECheck = skip;
  }

  void setIncomingECS(boost::optional<const EDNSSubnetOpts&> incomingECS)
  {
    d_incomingECS = incomingECS;
  }

#ifdef HAVE_PROTOBUF
  void setInitialRequestId(boost::optional<const boost::uuids::uuid&> initialRequestId)
  {
    d_initialRequestId = initialRequestId;
  }
#endif

  void setAsyncCallback(asyncresolve_t func)
  {
    d_asyncResolve = func;
  }

  static thread_local ThreadLocalStorage t_sstorage;

  static std::atomic<uint64_t> s_queries;
  static std::atomic<uint64_t> s_outgoingtimeouts;
  static std::atomic<uint64_t> s_outgoing4timeouts;
  static std::atomic<uint64_t> s_outgoing6timeouts;
  static std::atomic<uint64_t> s_throttledqueries;
  static std::atomic<uint64_t> s_dontqueries;
  static std::atomic<uint64_t> s_outqueries;
  static std::atomic<uint64_t> s_tcpoutqueries;
  static std::atomic<uint64_t> s_nodelegated;
  static std::atomic<uint64_t> s_unreachables;

  static string s_serverID;
  static unsigned int s_minimumTTL;
  static unsigned int s_maxqperq;
  static unsigned int s_maxtotusec;
  static unsigned int s_maxdepth;
  static unsigned int s_maxnegttl;
  static unsigned int s_maxcachettl;
  static unsigned int s_packetcachettl;
  static unsigned int s_packetcacheservfailttl;
  static unsigned int s_serverdownmaxfails;
  static unsigned int s_serverdownthrottletime;
  static uint8_t s_ecsipv4limit;
  static uint8_t s_ecsipv6limit;
  static bool s_doIPv6;
  static bool s_noEDNSPing;
  static bool s_noEDNS;
  static bool s_rootNXTrust;
  static bool s_nopacketcache;

  std::unordered_map<std::string,bool> d_discardedPolicies;
  DNSFilterEngine::Policy d_appliedPolicy;
  unsigned int d_outqueries;
  unsigned int d_tcpoutqueries;
  unsigned int d_throttledqueries;
  unsigned int d_timeouts;
  unsigned int d_unreachables;
  unsigned int d_totUsec;
  ComboAddress d_requestor;

private:

  static std::unordered_set<DNSName> s_delegationOnly;
  static NetmaskGroup s_ednssubnets;
  static SuffixMatchNode s_ednsdomains;
  static LogMode s_lm;
  static std::unique_ptr<NetmaskGroup> s_dontQuery;

  struct GetBestNSAnswer
  {
    DNSName qname;
    set<pair<DNSName,DNSName> > bestns;
    uint8_t qtype; // only A and AAAA anyhow
    bool operator<(const GetBestNSAnswer &b) const
    {
      return boost::tie(qname, qtype, bestns) <
	boost::tie(b.qname, b.qtype, b.bestns);
    }
  };

  int doResolveAt(NsSet &nameservers, DNSName auth, bool flawedNSSet, const DNSName &qname, const QType &qtype, vector<DNSRecord>&ret,
                  unsigned int depth, set<GetBestNSAnswer>&beenthere);
  bool doResolveAtThisIP(const std::string& prefix, const DNSName& qname, const QType& qtype, LWResult& lwr, boost::optional<Netmask>& ednsmask, const DNSName& auth, bool const sendRDQuery, const DNSName& nsName, const ComboAddress& remoteIP, bool doTCP, bool* truncated);
  bool processAnswer(unsigned int depth, LWResult& lwr, const DNSName& qname, const QType& qtype, DNSName& auth, bool wasForwarded, const boost::optional<Netmask> ednsmask, bool sendRDQuery, NsSet &nameservers, std::vector<DNSRecord>& ret, const DNSFilterEngine& dfe, bool* gotNewServers, int* rcode);

  int doResolve(const DNSName &qname, const QType &qtype, vector<DNSRecord>&ret, unsigned int depth, set<GetBestNSAnswer>& beenthere);
  bool doOOBResolve(const AuthDomain& domain, const DNSName &qname, const QType &qtype, vector<DNSRecord>&ret, int& res) const;
  bool doOOBResolve(const DNSName &qname, const QType &qtype, vector<DNSRecord>&ret, unsigned int depth, int &res);
  domainmap_t::const_iterator getBestAuthZone(DNSName* qname) const;
  bool doCNAMECacheCheck(const DNSName &qname, const QType &qtype, vector<DNSRecord>&ret, unsigned int depth, int &res);
  bool doCacheCheck(const DNSName &qname, const QType &qtype, vector<DNSRecord>&ret, unsigned int depth, int &res);
  void getBestNSFromCache(const DNSName &qname, const QType &qtype, vector<DNSRecord>&bestns, bool* flawedNSSet, unsigned int depth, set<GetBestNSAnswer>& beenthere);
  DNSName getBestNSNamesFromCache(const DNSName &qname, const QType &qtype, NsSet& nsset, bool* flawedNSSet, unsigned int depth, set<GetBestNSAnswer>&beenthere);

  inline vector<DNSName> shuffleInSpeedOrder(NsSet &nameservers, const string &prefix);
  bool moreSpecificThan(const DNSName& a, const DNSName &b) const;
  vector<ComboAddress> getAddrs(const DNSName &qname, unsigned int depth, set<GetBestNSAnswer>& beenthere);

  bool nameserversBlockedByRPZ(const DNSFilterEngine& dfe, const NsSet& nameservers);
  bool nameserverIPBlockedByRPZ(const DNSFilterEngine& dfe, const ComboAddress&);
  bool throttledOrBlocked(const std::string& prefix, const ComboAddress& remoteIP, const DNSName& qname, const QType& qtype, bool pierceDontQuery);

  vector<ComboAddress> retrieveAddressesForNS(const std::string& prefix, const DNSName& qname, vector<DNSName >::const_iterator& tns, const unsigned int depth, set<GetBestNSAnswer>& beenthere, const vector<DNSName >& rnameservers, NsSet& nameservers, bool& sendRDQuery, bool& pierceDontQuery, bool& flawedNSSet);
  RCode::rcodes_ updateCacheFromRecords(const std::string& prefix, LWResult& lwr, const DNSName& qname, const DNSName& auth, bool wasForwarded, const boost::optional<Netmask>);
  bool processRecords(const std::string& prefix, const DNSName& qname, const QType& qtype, const DNSName& auth, LWResult& lwr, const bool sendRDQuery, vector<DNSRecord>& ret, set<DNSName>& nsset, DNSName& newtarget, DNSName& newauth, bool& realreferral, bool& negindic);

  bool doSpecialNamesResolve(const DNSName &qname, const QType &qtype, const uint16_t qclass, vector<DNSRecord> &ret);

  int asyncresolveWrapper(const ComboAddress& ip, bool ednsMANDATORY, const DNSName& domain, int type, bool doTCP, bool sendRDQuery, struct timeval* now, boost::optional<Netmask>& srcmask, LWResult* res) const;

  boost::optional<Netmask> getEDNSSubnetMask(const ComboAddress& local, const DNSName&dn, const ComboAddress& rem);

  void setUpdatingRootNS()
  {
    d_updatingRootNS = true;
  }


  ostringstream d_trace;
  shared_ptr<RecursorLua4> d_pdl;
  boost::optional<const EDNSSubnetOpts&> d_incomingECS;
#ifdef HAVE_PROTOBUF
  boost::optional<const boost::uuids::uuid&> d_initialRequestId;
#endif
  asyncresolve_t d_asyncResolve{nullptr};
  struct timeval d_now;
  string d_prefix;

  /* When d_cacheonly is set to true, we will only check the cache.
   * This is set when the RD bit is unset in the incoming query
   */
  bool d_cacheonly;
  bool d_doDNSSEC;
  bool d_doEDNS0{true};
  bool d_incomingECSFound{false};
  bool d_skipCNAMECheck{false};
  bool d_updatingRootNS{false};
  bool d_wantsRPZ{true};
  bool d_wasOutOfBand{false};
  bool d_wasVariable{false};

  LogMode d_lm;
};

class Socket;
/* external functions, opaque to us */
int asendtcp(const string& data, Socket* sock);
int arecvtcp(string& data, size_t len, Socket* sock, bool incompleteOkay);


struct PacketID
{
  PacketID() : id(0), type(0), sock(0), inNeeded(0), inIncompleteOkay(false), outPos(0), nearMisses(0), fd(-1)
  {
    memset(&remote, 0, sizeof(remote));
  }

  uint16_t id;  // wait for a specific id/remote pair
  uint16_t type;             // and this is its type
  ComboAddress remote;  // this is the remote
  DNSName domain;             // this is the question

  Socket* sock;  // or wait for an event on a TCP fd
  string inMSG; // they'll go here
  size_t inNeeded; // if this is set, we'll read until inNeeded bytes are read
  bool inIncompleteOkay;

  string outMSG; // the outgoing message that needs to be sent
  string::size_type outPos;    // how far we are along in the outMSG

  typedef set<uint16_t > chain_t;
  mutable chain_t chain;
  mutable uint32_t nearMisses; // number of near misses - host correct, id wrong
  int fd;

  bool operator<(const PacketID& b) const
  {
    int ourSock= sock ? sock->getHandle() : 0;
    int bSock = b.sock ? b.sock->getHandle() : 0;
    if( tie(remote, ourSock, type) < tie(b.remote, bSock, b.type))
      return true;
    if( tie(remote, ourSock, type) > tie(b.remote, bSock, b.type))
      return false;

    return tie(domain, fd, id) < tie(b.domain, b.fd, b.id);
  }
};

struct PacketIDBirthdayCompare: public std::binary_function<PacketID, PacketID, bool>
{
  bool operator()(const PacketID& a, const PacketID& b) const
  {
    int ourSock= a.sock ? a.sock->getHandle() : 0;
    int bSock = b.sock ? b.sock->getHandle() : 0;
    if( tie(a.remote, ourSock, a.type) < tie(b.remote, bSock, b.type))
      return true;
    if( tie(a.remote, ourSock, a.type) > tie(b.remote, bSock, b.type))
      return false;

    return a.domain < b.domain;
  }
};
extern thread_local std::unique_ptr<MemRecursorCache> t_RC;
extern thread_local std::unique_ptr<RecursorPacketCache> t_packetCache;
typedef MTasker<PacketID,string> MT_t;
MT_t* getMT();

struct RecursorStats
{
  std::atomic<uint64_t> servFails;
  std::atomic<uint64_t> nxDomains;
  std::atomic<uint64_t> noErrors;
  std::atomic<uint64_t> answers0_1, answers1_10, answers10_100, answers100_1000, answersSlow;
  std::atomic<uint64_t> auth4Answers0_1, auth4Answers1_10, auth4Answers10_100, auth4Answers100_1000, auth4AnswersSlow;
  std::atomic<uint64_t> auth6Answers0_1, auth6Answers1_10, auth6Answers10_100, auth6Answers100_1000, auth6AnswersSlow;
  double avgLatencyUsec;
  std::atomic<uint64_t> qcounter;     // not increased for unauth packets
  std::atomic<uint64_t> ipv6qcounter;
  std::atomic<uint64_t> tcpqcounter;
  std::atomic<uint64_t> unauthorizedUDP;  // when this is increased, qcounter isn't
  std::atomic<uint64_t> unauthorizedTCP;  // when this is increased, qcounter isn't
  std::atomic<uint64_t> policyDrops;
  std::atomic<uint64_t> tcpClientOverflow;
  std::atomic<uint64_t> clientParseError;
  std::atomic<uint64_t> serverParseError;
  std::atomic<uint64_t> tooOldDrops;
  std::atomic<uint64_t> unexpectedCount;
  std::atomic<uint64_t> caseMismatchCount;
  std::atomic<uint64_t> spoofCount;
  std::atomic<uint64_t> resourceLimits;
  std::atomic<uint64_t> overCapacityDrops;
  std::atomic<uint64_t> ipv6queries;
  std::atomic<uint64_t> chainResends;
  std::atomic<uint64_t> nsSetInvalidations;
  std::atomic<uint64_t> ednsPingMatches;
  std::atomic<uint64_t> ednsPingMismatches;
  std::atomic<uint64_t> noPingOutQueries, noEdnsOutQueries;
  std::atomic<uint64_t> packetCacheHits;
  std::atomic<uint64_t> noPacketError;
  std::atomic<uint64_t> ignoredCount;
  time_t startupTime;
  std::atomic<uint64_t> dnssecQueries;
  unsigned int maxMThreadStackUsage;
  std::atomic<uint64_t> dnssecValidations; // should be the sum of all dnssecResult* stats
  std::map<vState, std::atomic<uint64_t> > dnssecResults;
  std::map<DNSFilterEngine::PolicyKind, std::atomic<uint64_t> > policyResults;
};

//! represents a running TCP/IP client session
class TCPConnection : public boost::noncopyable
{
public:
  TCPConnection(int fd, const ComboAddress& addr);
  ~TCPConnection();

  int getFD() const
  {
    return d_fd;
  }
  enum stateenum {BYTE0, BYTE1, GETQUESTION, DONE} state{BYTE0};
  uint16_t qlen{0};
  uint16_t bytesread{0};
  const ComboAddress d_remote;
  char data[65535]; // damn
  size_t queriesCount{0};

  static unsigned int getCurrentConnections() { return s_currentConnections; }
private:
  const int d_fd;
  static AtomicCounter s_currentConnections; //!< total number of current TCP connections
};

class ImmediateServFailException
{
public:
  ImmediateServFailException(string r){reason=r;};

  string reason; //! Print this to tell the user what went wrong
};

#if (__GNUC__ == 4 && __GNUC_MINOR__ == 2)
typedef boost::circular_buffer<SComboAddress> addrringbuf_t;
#else
typedef boost::circular_buffer<ComboAddress> addrringbuf_t;
#endif
extern thread_local std::unique_ptr<addrringbuf_t> t_servfailremotes, t_largeanswerremotes, t_remotes;

extern thread_local std::unique_ptr<boost::circular_buffer<pair<DNSName,uint16_t> > > t_queryring, t_servfailqueryring;
extern thread_local std::shared_ptr<NetmaskGroup> t_allowFrom;
string doQueueReloadLuaScript(vector<string>::const_iterator begin, vector<string>::const_iterator end);
string doTraceRegex(vector<string>::const_iterator begin, vector<string>::const_iterator end);
void parseACLs();
extern RecursorStats g_stats;
extern unsigned int g_numThreads;
extern uint16_t g_outgoingEDNSBufsize;


std::string reloadAuthAndForwards();
ComboAddress parseIPAndPort(const std::string& input, uint16_t port);
ComboAddress getQueryLocalAddress(int family, uint16_t port, int fd);
typedef boost::function<void*(void)> pipefunc_t;
void broadcastFunction(const pipefunc_t& func, bool skipSelf = false);
void distributeAsyncFunction(const std::string& question, const pipefunc_t& func);

int directResolve(const DNSName& qname, const QType& qtype, int qclass, vector<DNSRecord>& ret);

template<class T> T broadcastAccFunction(const boost::function<T*()>& func, bool skipSelf=false);

std::shared_ptr<SyncRes::domainmap_t> parseAuthAndForwards();
uint64_t* pleaseGetNsSpeedsSize();
uint64_t* pleaseGetCacheSize();
uint64_t* pleaseGetNegCacheSize();
uint64_t* pleaseGetCacheHits();
uint64_t* pleaseGetCacheMisses();
uint64_t* pleaseGetConcurrentQueries();
uint64_t* pleaseGetThrottleSize();
uint64_t* pleaseGetPacketCacheHits();
uint64_t* pleaseGetPacketCacheSize();
uint64_t* pleaseWipeCache(const DNSName& canon, bool subtree=false);
uint64_t* pleaseWipePacketCache(const DNSName& canon, bool subtree);
uint64_t* pleaseWipeAndCountNegCache(const DNSName& canon, bool subtree=false);
void doCarbonDump(void*);
void primeHints(void);

extern __thread struct timeval g_now;

#ifdef HAVE_PROTOBUF
extern thread_local std::unique_ptr<boost::uuids::random_generator> t_uuidGenerator;
#endif
