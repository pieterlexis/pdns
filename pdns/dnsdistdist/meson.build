project('dnsdist', 'cpp')
conf_data = configuration_data()
conf_data.set('VERSION', '"0.0.1"')
conf_data.set('SYSCONFDIR', join_paths(get_option('prefix'), get_option('sysconfdir')))

configure_file(output : 'config.h',
               configuration : conf_data)

# TODO detect BPS support (ext/libbpf)
include_dirs = include_directories('ext/incbin', 'ext/json11', 'ext/luawrapper/include', 'ext/yahttp')

if get_option('with-luajit')
  luadep = dependency('luajit')
else
  luadep = dependency('lua')
endif

sources = ['ascii.hh',
  'base32.hh',
  'base64.hh',
  'bpf-filter.cc',
  'delaypipe.cc',
  #  'devpollmplexer.cc',
  'dns.cc',
  'dnscrypt.cc',
  'dnsdist-cache.cc',
  'dnsdist-carbon.cc',
  'dnsdist.cc',
  'dnsdist-console.cc',
  'dnsdist-dnscrypt.cc',
  'dnsdist-dynbpf.cc',
  'dnsdist-ecs.cc',
  'dnsdist.hh',
  'dnsdist-lua-actions.cc',
  'dnsdist-lua-bindings.cc',
  'dnsdist-lua-bindings-dnsquestion.cc',
  'dnsdist-lua.cc',
  'dnsdist-lua-inspection.cc',
  'dnsdist-lua-rules.cc',
  'dnsdist-lua-vars.cc',
  'dnsdist-protobuf.cc',
  'dnsdist-rings.cc',
  'dnsdist-snmp.cc',
  'dnsdist-tcp.cc',
  'dnsdist-web.cc',
  'dnsname.cc',
  'dnsparser.cc',
  'dnstap.cc',
  'dnswriter.cc',
  'ednscookies.cc',
  'ednsoptions.cc',
  'ednssubnet.cc',
  'epollmplexer.cc',
  'fstrm_logger.cc',
  'gettime.cc',
  'iputils.cc',
  'kqueuemplexer.cc',
  'misc.cc',
  'pollmplexer.cc',
  'portsmplexer.cc',
  'protobuf.cc',
  'qtype.cc',
  'remote_logger.cc',
  'snmp-agent.cc',
  'sodcrypto.cc',
  'sstuff.hh',
  'statnode.cc',
  'tcpiohandler.cc',
  'xpf.cc']

openssldep = dependency('libcrypto')

if get_option('enable-protobuf') or get_option('enable-dnstap')
  protobufdep = dependency('protobuf')
  protoc = find_program('protoc')
  protoc_gen = generator(protoc, \
      output:     ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
      arguments : ['--proto_path=@SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '@INPUT@'])
endif

protobuf_files = []
if get_option('enable-protobuf')
  sources += protoc_gen.process('dnsmessage.proto')
endif

dnstapdep = dependency('fstrm', required: false)

if get_option('enable-dnstap')
  if not dnstapdep.found()

  sources += protoc_gen.process('dnstap.proto')
  dependencies += dnstapdep
endif

executable('dnsdist',
  sources,
  dependencies: [dnstapdep,
                 openssldep,
                 protobufdep])
