project('dnsdist', 'cpp', default_options: ['cpp_std=c++11'], license: 'GPL2')
conf_data = configuration_data()
conf_data.set_quoted('VERSION', '0.0.1')
conf_data.set_quoted('SYSCONFDIR', get_option('sysconfdir'))

include_dirs = include_directories('ext/incbin', 'ext/json11', 'ext/luawrapper/include', 'ext/yahttp')

sources = ['ascii.hh',
  'base32.hh',
  'base64.hh',
  'bpf-filter.cc',
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

custom_target('htmlfiles.h',
  command: ['@SOURCE_DIR@/incfiles', '@SOURCE_DIR@'],
  build_always: true,
  output: 'htmlfiles.h')

luadep = dependency('luajit', required: get_option('luajit'))
luadep = dependency('lua', required: not get_option('luajit'))

need_sodium = get_option('console-encryption') or get_option('dnscrypt')
need_protobuf = get_option('protobuf') or get_option('dnstap')
need_openssl = get_option('dns-over-tls') and required('openssl')
need_gnutls = get_option('dns-over-tls') and required('gnutls')

boostdep = dependency('boost', version: '>=1.42')
re2dep = dependency('re2', required: get_option('re2'))
openssldep = dependency('openssl', required: need_openssl)
gnutlsdep = dependency('gnutls', required: need_gnutls)
sodiumdep = dependency('libsodium', required: need_sodium)
dnstapdep = dependency('fstrm', required: get_option('dnstap'))
systemddep = dependency('systemd', required: get_option('systemd'))
protobufdep = dependency('protobuf', required: need_protobuf)
protoc = find_program('protoc', required: need_protobuf)

compiler = meson.get_compiler('cpp')

if get_option('ebpf')
  if not compiler.has_header('linux/bpf.h')
    error('linux/bpf.h not found, can not enable eBPF support!')
  endif
  include_dirs += 'ext/libbpf'
endif

protoc_gen = generator(protoc,
    output:     ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
    arguments : ['--proto_path=@SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '@INPUT@'])

if get_option('protobuf') and protobufdep.found() and protoc.found()
  conf_data.set('HAVE_PROTOBUF', 1)
  sources += protoc_gen.process('dnsmessage.proto')
endif

if get_option('dnstap') and dnstapdep.found() and protobufdep.found() and protoc.found()
  conf_data.set('HAVE_FSTRM', 1)
  sources += protoc_gen.process('dnstap.proto')
endif

configure_file(output : 'config.h',
               configuration : conf_data)

executable('dnsdist',
  sources,
  include_directories: include_dirs,
  dependencies: [dnstapdep,
                 openssldep,
                 re2dep,
                 boostdep,
                 protobufdep,
                 sodiumdep,
                 luadep,
                 gnutlsdep,
                 systemddep
                 ])
