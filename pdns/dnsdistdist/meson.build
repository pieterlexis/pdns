project('dnsdist', 'cpp',
  version: run_command('build-aux/gen-version').stdout().strip(),
  default_options : ['cpp_std=c++17'],
  license: ['GPLv2'],
  meson_version: '>=0.52'
)

null_dep = dependency('', required: false)

add_global_arguments('-DHAVE_CONFIG_H=1', language : 'cpp')
compiler = meson.get_compiler('cpp')

conf = configuration_data()
conf.set('DNSDIST', 1)
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('PACKAGE_STRING', meson.project_name() + ' ' + meson.project_version())
conf.set_quoted('SYSCONFDIR', join_paths(get_option('prefix'), get_option('sysconfdir')))

#########
# HELPER PROGRAMS
#########
ragel = find_program('ragel')
ragel_gen = generator(ragel,
  output: '@BASENAME@.cc',
  arguments: ['@INPUT@', '-o', '@OUTPUT@'])
dnslabeltext_src = ragel_gen.process('dnslabeltext.rl')

#########
# LIBRARIES AND FUNCTIONS
#########
thread_dep = dependency('threads')
foreach f : ['pthread_setaffinity_np', 'pthread_getattr_np', 'pthread_get_stackaddr_np', 'pthread_get_stacksize_np']
  if compiler.has_function(f, dependencies: thread_dep)
    conf.set('HAVE_' + f.to_upper(), 1)
  endif
endforeach
## TODO pthread_set_name detection

if target_machine.system() == 'linux'
  conf.set('HAVE_LINUX', 1)
  poll_sources = files('epollmplexer.cc')
endif
if target_machine.system() == 'freebsd'
  conf.set('HAVE_FREEBSD', 1)
  poll_sources = files('kqueuemplexer.cc')
endif
if target_machine.system() == 'sunos'
  conf.set('HAVE_SOLARIS', 1)
  poll_sources = files('devpollmplexer.cc', 'portsmplexer.cc')
endif

if compiler.links('''
#include <stdint.h>
int main(void) {
  uint64_t val = 0;
  __atomic_add_fetch(&val, 1, __ATOMIC_RELAXED);
  return 0;
}''', name : 'built-in atomics')
  atomic_dep = null_dep
else
  atomic_dep = cc.find_library('atomic')
endif

foreach f : ['recvmmsg', 'sendmmsg', 'accept4', 'explicit_bzero', 'explicit_memset']
  if compiler.has_function(f)
    conf.set('HAVE_' + f.to_upper(), 1)
  endif
endforeach

resolv_dep = dependency('resolv', required: false)
if not compiler.has_function('inet_aton', dependencies: resolv_dep)
  error('no "inet_aton()" found')
endif

nsl_dep = dependency('nsl', required: false)
foreach f : ['gethostbyname', 'gethostent']
  if not compiler.has_function(f, dependencies: nsl_dep)
    error('no "' + f + '()" found')
  endif
endforeach

socket_dep = dependency('socket', required: false)
if not compiler.has_function('socket', dependencies: socket_dep)
  error('no "socket()" found')
endif

libedit_dep = dependency('libedit')

if compiler.has_function('clock_gettime')
  conf.set('HAVE_CLOCK_GETTIME', 1)
endif

boost_dep = dependency('boost')
conf.set('HAVE_BOOST', 1)

libcap_dep = dependency('libcap')
if libcap_dep.found()
  conf.set('HAVE_LIBCAP', 1)
endif

libsystemd_dep = dependency('libsystemd')
if libcap_dep.found()
  conf.set('HAVE_SYSTEMD', 1)
endif

openssl_dep = dependency('openssl', required: false)
if openssl_dep.found()
  conf.set('HAVE_LIBCRYPTO', 1)
  conf.set('HAVE_LIBSSL', 1)
  foreach f : ['SSL_CTX_set_ciphersuites', 'OCSP_basic_sign', 'SSL_CTX_set_num_tickets', 'SSL_CTX_set_keylog_callback', 'SSL_CTX_get0_privatekey', 'SSL_CTX_set_min_proto_version']
    if compiler.has_function(f, dependencies: openssl_dep)
      conf.set('HAVE_' + f.to_upper(), 1)
    endif
  endforeach
endif

gnutls_dep = dependency('gnutls', required: false)
if gnutls_dep.found()
  conf.set('HAVE_GNUTLS', 1)
endif

if openssl_dep.found() or gnutls_dep.found()
  conf.set('HAVE_DNS_OVER_TLS', 1)
endif

libh2o_evloop_dep = dependency('libh2o-evloop', required: false)
if libh2o_evloop_dep.found()
  conf.set('HAVE_LIBH2OEVLOOP', 1)
  if compiler.has_function('h2o_socket_get_ssl_server_name', prefix: '#include <h2o/socket.h>')
    conf.set('HAVE_H2O_SOCKET_GET_SSL_SERVER_NAME', 1)
  endif
endif

if libh2o_evloop_dep.found() and openssl_dep.found()
  conf.set('HAVE_DNS_OVER_HTTPS', 1)
endif

lmdb_dep = dependency('lmdb', required: false)
if lmdb_dep.found()
  if compiler.has_header('lmdb.h', dependencies: [lmdb_dep]) and compiler.has_function('mdb_env_open', dependencies: [lmdb_dep], prefix: '#include <lmdb.h>')
    conf.set('HAVE_LMDB', 1)
    lmdb_safe_sources = files('ext/lmdb-safe/lmdb-safe.cc')
    lmdb_safe_includes = include_directories('ext/lmdb-safe')
    lmdb_safe_dep = declare_dependency(sources: lmdb_safe_sources, include_directories: [lmdb_safe_includes])
  endif
endif

cdb_dep = dependency('cdb', required: false)
if not cdb_dep.found()
  cdb_dep = compiler.find_library('cdb')
endif

if cdb_dep.found()
  cdb_files = files('cdb.cc')
  conf.set('HAVE_CDB', 1)
endif

#########
# Lua
#########
lua_dep = dependency('luajit', version: '>=2.0.2', required: false)
if not lua_dep.found()
  lua_dep = dependency('lua', version: '>=5.1', required: true)
endif

make_dnsdist_lua_ffi_prog = find_program('make-dnsdist-lua-ffi-interface.inc.sh')

lua_ffi_interface_src = custom_target('dnsdist-lua-ffi-interface.inc',
  input: 'dnsdist-lua-ffi-interface.h',
  output: 'dnsdist-lua-ffi-interface.inc',
  command: [make_dnsdist_lua_ffi_prog, '@INPUT@', '@OUTPUT@'])

###########
# YAHTTP
###########
foreach f : ['localtime_r', 'gmtime_r']
  if compiler.has_function(f)
    conf.set('HAVE_' + f.to_upper(), 1)
  endif
endforeach

yahttp_sources = files(
  'ext/yahttp/yahttp/reqresp.cpp',
  'ext/yahttp/yahttp/router.cpp'
)

yahttp_includes = include_directories('ext/yahttp')
yahttp_dep = declare_dependency(sources: yahttp_sources, include_directories: [yahttp_includes])

###########
# JSON 11
###########
json11_sources = files (
  'ext/json11/json11.cpp'
)
json11_includes = include_directories('ext/json11')
json11_dep = declare_dependency(sources: json11_sources, include_directories: [json11_includes])

############
# IPCRYPT
############
ipcrypt_sources = files('ext/ipcrypt/ipcrypt.cc')
ipcrypt_includes = include_directories('ext/ipcrypt')
ipcrypt_dep = declare_dependency(sources: ipcrypt_sources, include_directories: [ipcrypt_includes])

############
# HTML files
############

incfiles = find_program('incfiles')
htmlfiles_h_src = custom_target('htmlfiles.h',
  input: [
    'html/detail.css',
    'html/graph.css',
    'html/index.html',
    'html/js/d3.min.js',
    'html/js/jquery.min.js',
    'html/js/moment.min.js',
    'html/js/rickshaw.min.js',
    'html/legend.css',
    'html/lines.css',
    'html/local.js',
    'html/powerdns-logo-220px.png'
  ],
  output: ['htmlfiles.h'],
  command: [incfiles, meson.current_source_dir(), '@OUTPUT@'])


############
# OPTIONAL DEPENDENCIES
############

libsodium_dep = dependency('libsodium', required: false)
if libsodium_dep.found()
  conf.set('HAVE_LIBSODIUM', 1)
  foreach f : ['crypto_box_easy_afternm', 'crypto_box_curve25519xchacha20poly1305_easy', 'randombytes_stir']
    if compiler.has_function(f, dependencies: libsodium_dep)
      conf.set('HAVE_' + f.to_upper(), 1)
    endif
  endforeach
endif

libfstrm_dep = dependency('libfstrm', required: false)
if libfstrm_dep.found()
  conf.set('HAVE_LIBFSTRM', 1)
  if compiler.has_function('fstrm_tcp_writer_init', dependencies: libfstrm_dep)
    conf.set('HAVE_FSTRM_TCP_WRITER_INIT', 1)
  endif
endif

config_h = configure_file(output: 'config.h', configuration: conf)

dnsdist_sources = files(
  'bpf-filter.cc',
  'capabilities.cc',
  'dns.cc',
  'dnscrypt.cc',
  'dnsdist-backend.cc',
  'dnsdist-cache.cc',
  'dnsdist-carbon.cc',
  'dnsdist.cc',
  'dnsdist-console.cc',
  'dnsdist-dnscrypt.cc',
  'dnsdist-dynblocks.cc',
  'dnsdist-dynbpf.cc',
  'dnsdist-ecs.cc',
  'dnsdist-healthchecks.cc',
  'dnsdist-idstate.cc',
  'dnsdist-kvs.cc',
  'dnsdist-lbpolicies.cc',
  'dnsdist-lua-actions.cc',
  'dnsdist-lua-bindings.cc',
  'dnsdist-lua-bindings-dnscrypt.cc',
  'dnsdist-lua-bindings-dnsquestion.cc',
  'dnsdist-lua-bindings-kvs.cc',
  'dnsdist-lua-bindings-packetcache.cc',
  'dnsdist-lua-bindings-protobuf.cc',
  'dnsdist-lua.cc',
  'dnsdist-lua-ffi.cc',
  'dnsdist-lua-inspection.cc',
  'dnsdist-lua-inspection-ffi.cc',
  'dnsdist-lua-rules.cc',
  'dnsdist-lua-vars.cc',
  'dnsdist-lua-web.cc',
  'dnsdist-protobuf.cc',
  'dnsdist-proxy-protocol.cc',
  'dnsdist-rings.cc',
  'dnsdist-secpoll.cc',
  'dnsdist-snmp.cc',
  'dnsdist-systemd.cc',
  'dnsdist-tcp.cc',
  'dnsdist-tcp-downstream.cc',
  'dnsdist-web.cc',
  'dnsdist-xpf.cc',
  'dnsname.cc',
  'dnsparser.cc',
  'dnstap.cc',
  'dnswriter.cc',
  'doh.cc',
  'ednscookies.cc',
  'ednsoptions.cc',
  'ednssubnet.cc',
  'fstrm_logger.cc',
  'gettime.cc',
  'ipcipher.cc',
  'iputils.cc',
  'libssl.cc',
  'misc.cc',
  'pollmplexer.cc',
  'protozero.cc',
  'proxy-protocol.cc',
  'qtype.cc',
  'remote_logger.cc',
  'snmp-agent.cc',
  'sodcrypto.cc',
  'statnode.cc',
  'svc-records.cc',
  'tcpiohandler.cc',
  'threadname.cc',
  'uuid-utils.cc',
  'xpf.cc'
)

executable('dnsdist',
  sources: [
    cdb_files,
    config_h,
    dnsdist_sources,
    dnslabeltext_src,
    htmlfiles_h_src,
    lua_ffi_interface_src,
    poll_sources
  ],
  dependencies: [
    atomic_dep,
    boost_dep,
    cdb_dep,
    gnutls_dep,
    ipcrypt_dep,
    json11_dep,
    libcap_dep,
    libedit_dep,
    libfstrm_dep,
    libh2o_evloop_dep,
    libsodium_dep,
    libsystemd_dep,
    lmdb_dep,
    lmdb_safe_dep,
    lua_dep,
    nsl_dep,
    openssl_dep,
    resolv_dep,
    socket_dep,
    thread_dep,
    yahttp_dep
  ],
  include_directories: [
    ipcrypt_includes,
    json11_includes,
    yahttp_includes
  ]
)
