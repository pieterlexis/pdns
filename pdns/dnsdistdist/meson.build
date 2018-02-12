project('dnsdist', 'cpp',
  default_options: ['cpp_std=c++11', 'buildtype=debugoptimized'],
  license: 'GPL2',
  version: '0.0.0')
conf_data = configuration_data()
conf_data.set_quoted('VERSION', meson.project_version())
conf_data.set_quoted('SYSCONFDIR', get_option('sysconfdir'))
conf_data.set_quoted('PACKAGE_STRING', meson.project_name() + '-' + meson.project_version())

include_dirs = include_directories('ext/incbin', 'ext/json11', 'ext/luawrapper/include', 'ext/yahttp')

sources_json11 = ['ext/json11/json11.cpp', 'ext/json11/json11.hpp']
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
  'dnsmessage.proto',
  'dnstap.proto',
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
  command: ['@SOURCE_DIR@/incfiles', '@SOURCE_DIR@', '@OUTPUT@'],
  build_always: true,
  output: 'htmlfiles.h')

if get_option('luajit')
  luadep = dependency('luajit')
else
  luadep = dependency('lua')
endif

need_sodium = get_option('console-encryption') or get_option('dnscrypt')
need_protobuf = get_option('protobuf')
need_openssl = get_option('dns-over-tls') and required('openssl')
need_gnutls = get_option('dns-over-tls') and required('gnutls')

boostdep = dependency('boost', version: '>=1.42')
re2dep = dependency('re2', required: get_option('re2'))
openssldep = dependency('openssl', required: need_openssl)
gnutlsdep = dependency('gnutls', required: need_gnutls)
sodiumdep = dependency('libsodium', required: need_sodium)
dnstapdep = dependency('fstrm', required: get_option('dnstap-fstrm'))
systemddep = dependency('libsystemd', required: get_option('systemd'))
libeditdep = dependency('libedit')
protobufdep = dependency('protobuf', required: need_protobuf)
protoc = find_program('protoc', required: need_protobuf)

compiler = meson.get_compiler('cpp')

# security stuff
if get_option('hardening')
  foreach arg : ['-Wl,-pie', '-fPIE -DPIE', '-pie',
                 '-fstack-protector',
                 '--param ssp-buffer-size=4',
                 '-D_FORTIFY_SOURCE=2',
                 '-Wl,-z,relro', '-Wl,-z,now']
    if compiler.has_argument(arg)
      add_global_arguments(arg, language: 'cpp')
    endif
  endforeach
endif

if get_option('ebpf')
  if not compiler.has_header('linux/bpf.h')
    error('linux/bpf.h not found, can not enable eBPF support!')
  endif
  conf_data.set('HAVE_EBPF', 1)
  include_dirs += 'ext/libbpf'
endif

if sodiumdep.found()
  # Needed for yahttp
  conf_data.set('HAVE_LIBSODIUM', 1)
endif

if boostdep.found()
  # Needed for yahttp
  conf_data.set('HAVE_BOOST', 1)
endif

if systemddep.found()
  conf_data.set('HAVE_SYSTEMD', 1)
endif

if get_option('dnscrypt')
  conf_data.set('HAVE_DNSCRYPT', 1)
endif

if compiler.has_function('clock_gettime', prefix : '#include <time.h>')
  conf_data.set('HAVE_CLOCK_GETTIME', 1)
endif

protoc_gen = generator(protoc,
    output:     ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
    arguments : ['--proto_path=@SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '@INPUT@'])

if protobufdep.found() and protoc.found()
  conf_data.set('HAVE_PROTOBUF', 1)
  sources += protoc_gen.process('dnsmessage.proto')
  sources += protoc_gen.process('dnstap.proto')
  if get_option('dnstap-fstrm') and dnstapdep.found()
    conf_data.set('HAVE_FSTRM', 1)
  endif
endif

ragel = find_program('ragel')
ragel_gen = generator(ragel,
  output:    ['@BASENAME@.cc'],
  arguments: ['@INPUT@', '-o', '@OUTPUT@'])

sources += ragel_gen.process('dnslabeltext.rl')

json11 = static_library('json11', sources_json11)
yahttp = static_library('yahttp', sources_yahttp)

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
                 systemddep,
                 libeditdep],
  link_with: [json11, yahttp])
