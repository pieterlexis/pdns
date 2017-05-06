project('pdns-recursor', 'cpp', version: '0.0.0')
conf = configuration_data()
conf.set_quoted('VERSION', '0.0.0')
conf.set('RECURSOR', 1)
configure_file(output : 'config.h',
               configuration : conf)
add_global_arguments('-DHAVE_CONFIG_H=1', language : 'cpp')
add_global_arguments('-DHAVE_LUA=1', language : 'cpp')
add_global_arguments('-DSYSCONFDIR="/etc"', language : 'cpp')
add_global_arguments('-DLOCALSTATEDIR="/var/run"', language : 'cpp')
add_global_arguments('-DHAVE_CXX11=1', language : 'cpp')

inc_json11 = include_directories('ext/json11')
inc_yahttp = include_directories('ext/yahttp')
inc_incbin = include_directories('ext/incbin')
inc_lua = include_directories('ext/luawrapper/include')

dep_lua = dependency('lua')
dep_boost = dependency('boost', modules: ['thread', 'context'])
dep_openssl = dependency('openssl')
dep_protobuf = dependency('protobuf')
dep_systemd = dependency('systemd')

netsnmpconfig = find_program('net-snmp-config', required: false)
netsnmp_ldflags = ''
netsnmp_compileargs = ''
if netsnmpconfig.found()
  cmd = run_command(netsnmpconfig, '--agent-libs')
  netsnmp_ldflags = cmd.stdout().strip()
  cmd = run_command(netsnmpconfig, '--base-cflags')
  netsnmp_compileflags = cmd.stdout().strip()
endif

sources_json11 = ['ext/json11/json11.cpp', 'ext/json11/json11.hpp']
json11 = static_library('json11', sources_json11)

sources_yahttp = [
  'ext/yahttp/yahttp/cookie.hpp',
  'ext/yahttp/yahttp/exception.hpp',
  'ext/yahttp/yahttp/reqresp.cpp',
  'ext/yahttp/yahttp/reqresp.hpp',
  'ext/yahttp/yahttp/router.cpp',
  'ext/yahttp/yahttp/router.hpp',
  'ext/yahttp/yahttp/url.hpp',
  'ext/yahttp/yahttp/utility.hpp',
  'ext/yahttp/yahttp/yahttp-config.h',
  'ext/yahttp/yahttp/yahttp.hpp'
]
yahttp = static_library('yahttp', sources_yahttp)

sources = ['arguments.cc',
'ascii.hh',
'base32.cc',
'base32.hh',
'base64.cc',
'base64.hh',
'cachecleaner.hh',
'comment.hh',
'dns.hh',
'dns.cc',
'dns_random.hh',
'dns_random.cc',
'dnsbackend.hh',
## FIXME
'dnsmessage.pb.h',
'dnsmessage.pb.cc',
'dnslabeltext.cc',
'dnsname.cc',
'dnsname.hh',
'dnspacket.hh',
'dnsparser.hh',
'dnsparser.cc',
'dnsrecords.cc',
'dnsrecords.hh',
'dnssecinfra.hh',
'dnssecinfra.cc',
'dnsseckeeper.hh',
'dnswriter.cc',
'dnswriter.hh',
'ecs.cc',
'ednsoptions.cc',
'ednsoptions.hh',
'ednssubnet.cc',
'ednssubnet.hh',
'filterpo.cc',
'filterpo.hh',
'gettime.cc',
'gettime.hh',
'gss_context.cc',
'gss_context.hh',
'iputils.hh',
'iputils.cc',
'ixfr.cc',
'ixfr.hh',
'json.cc',
'json.hh',
'lazy_allocator.hh',
'lock.hh',
'logger.hh',
'logger.cc',
'lua-recursor4.cc',
'lua-recursor4.hh',
'lwres.cc',
'lwres.hh',
'misc.hh',
'misc.cc',
'mplexer.hh',
'mtasker.hh',
'mtasker_context.cc',
'mtasker_context.hh',
'namespaces.hh',
'negcache.hh',
'negcache.cc',
'nsecrecords.cc',
'opensslsigners.cc',
'opensslsigners.hh',
'packetcache.hh',
'pdns_recursor.cc',
'pdnsexception.hh',
'protobuf.cc',
'protobuf.hh',
'pubsuffix.hh',
'pubsuffix.cc',
'qtype.hh',
'qtype.cc',
'randomhelper.cc',
'rcpgenerator.cc',
'rcpgenerator.hh',
'rec-carbon.cc',
'rec-lua-conf.hh',
'rec-lua-conf.cc',
'rec-protobuf.cc',
'rec-protobuf.hh',
'rec-snmp.hh',
'rec-snmp.cc',
'rec_channel.cc',
'rec_channel.hh',
'rec_channel_rec.cc',
'recpacketcache.cc',
'recpacketcache.hh',
'recursor_cache.cc',
'recursor_cache.hh',
'reczones.cc',
'remote_logger.cc',
'remote_logger.hh',
'resolver.hh',
'resolver.cc',
'resolve-context.hh',
'responsestats.hh',
'responsestats.cc',
'root-addresses.hh',
'root-dnssec.hh',
'rpzloader.cc',
'rpzloader.hh',
'secpoll-recursor.cc',
'secpoll-recursor.hh',
'selectmplexer.cc',
'sholder.hh',
'sillyrecords.cc',
'snmp-agent.hh',
'snmp-agent.cc',
'sortlist.cc',
'sortlist.hh',
'sstuff.hh',
'syncres.cc',
'syncres.hh',
'tsigverifier.cc',
'tsigverifier.hh',
'ueberbackend.hh',
'unix_utility.cc',
'utility.hh',
'validate.cc',
'validate.hh',
'validate-recursor.cc',
'validate-recursor.hh',
'version.cc',
'version.hh',
'webserver.cc',
'webserver.hh',
'ws-api.cc',
'ws-api.hh',
'ws-recursor.cc',
'ws-recursor.hh',
'zoneparser-tng.cc',
'zoneparser-tng.hh']

executable('pdns-recursor', sources,
    include_directories: [inc_json11, inc_yahttp, inc_incbin],
    dependencies: [dep_lua, dep_boost, dep_openssl, dep_protobuf, dep_systemd],
    link_with: [json11, yahttp],
    compile_args: netsnmp_compileargs,
    link_args: '-Wl,-O1,--sort-common,--as-needed,-z,relro -L/usr/lib -lnetsnmpmibs -lpci -ldl -lnetsnmpagent -Wl,-rpath,/usr/lib/perl5/core_perl/CORE -lnetsnmp -lcrypto -lnl-3 -lm')

    #link_args : ' '.join(netsnmp_ldflags.split(' ')))
