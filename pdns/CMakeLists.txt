set(POLARSSL_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/ext/polarssl-1.1.2)
set(POLARSSL_LIBRARY ${POLARSSL_SUBDIR}/library/libpolarssl.a)
add_custom_target(build_polarssl ALL
  COMMAND ${CMAKE_MAKE_PROGRAM}
  WORKING_DIRECTORY ${POLARSSL_SUBDIR}
  COMMENT "Building bundled PolarSSL")
add_library(polarssl STATIC IMPORTED)
set_property(TARGET polarssl APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(polarssl PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${POLARSSL_LIBRARY}")
add_dependencies(polarssl build_polarssl)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)
BISON_TARGET(bindparser backends/bind/bindparser.yy ${CMAKE_CURRENT_BINARY_DIR}/bindparser.cc)
FLEX_TARGET(bindlexer backends/bind/bindlexer.l ${CMAKE_CURRENT_BINARY_DIR}/bindlexer.c)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
find_package(Ragel REQUIRED)
RAGEL_PARSER(dnslabeltext.rl)

add_custom_command(
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}/bind-dnssec.schema.sqlite3.sql.h"
  DEPENDS  bind-dnssec.schema.sqlite3.sql
  COMMAND echo 'static char sqlCreate[] __attribute__((unused))=' > ${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}/bind-dnssec.schema.sqlite3.sql.h
  COMMAND sed 's/$$/\"/g\;s/^/\"/g\;' bind-dnssec.schema.sqlite3.sql >> ${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}/bind-dnssec.schema.sqlite3.sql.h
  COMMAND echo '\;' >> ${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}/bind-dnssec.schema.sqlite3.sql.h
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Generating bind-dnssec.schema.sqlite3.sql"
)

add_custom_command(
  OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}/bindparser.h"
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}/bindparser.hh"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}/bindparser.hh" "${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}/bindparser.h"
  COMMENT "Copying bindparser.hh to bindparser.h"
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${SRCPATH}/version_generated.h "
#pragma once
#define PDNS_VERSION \"${VERSION}\"
#define DIST_HOST \"${DIST_HOST}\"
#define BUILD_DATE \"${BUILD_DATE}\"
#define BUILD_HOST \"${BUILD_HOST}\"
")

add_library(DNSUTILITY_LIB STATIC
  misc.cc misc.hh
  unix_utility.cc utility.hh
  qtype.cc qtype.hh
  logger.cc logger.hh
  statbag.cc statbag.hh
  dnsparser.cc dnsparser.hh
  version_generated.h
  aes/aescpp.h aes/aescrypt.c aes/aes.h aes/aeskey.c aes/aes_modes.c aes/aesopt.h aes/aestab.c aes/aestab.h aes/brg_endian.h aes/brg_types.h
  aes/dns_random.cc
  randomhelper.cc
  dns_random.hh
  arguments.cc arguments.hh
)

add_library(DNSPCAP_LIB STATIC
  dnspcap.cc dnspcap.hh
)

add_library(DNS_LIB STATIC
  base64.cc base64.hh
  dnsrecords.cc
  dnswriter.cc dnswriter.hh
  dnslabeltext.cc
  rcpgenerator.cc rcpgenerator.hh
  base32.cc
  sillyrecords.cc
  nsecrecords.cc
)

###############################################################################
# Auth
###############################################################################

add_library(PDNSAUTH_LIB STATIC
  ednssubnet.cc ednssubnet.hh
  ueberbackend.cc
  dnsbackend.cc
  dnspacket.cc dnspacket.hh
  dnsbackend.cc dnsbackend.hh
  zoneparser-tng.cc
  dns.cc
  dnssecinfra.cc dnssecinfra.hh
  polarrsakeyinfra.cc
  dbdnsseckeeper.cc
  dnssecsigner.cc
  signingpipe.cc cachecleaner.hh
  serialtweaker.cc
  packetcache.cc sstuff.hh
)

add_library(BINDBACKEND_LIB STATIC
  bind-dnssec.schema.sqlite3.sql.h
  backends/bind/bindbackend2.cc
  backends/bind/binddnssec.cc
  bindparser.h
  ${BISON_bindparser_OUTPUTS}
  ${FLEX_bindlexer_OUTPUTS}
)

add_library(GSQLBACKEND_LIB STATIC
  backends/gsql/gsqlbackend.cc
  backends/gsql/gsqlbackend.hh backends/gsql/ssql.hh
)

set(PDNS_SRCS
  nameserver.cc
  tcpreceiver.hh
  packethandler.cc
  tcpreceiver.cc
  pdnsexception.hh
  distributor.hh
  dns.hh
  dynmessenger.hh
  lock.hh
  nameserver.hh
  packetcache.hh
  packethandler.hh
  ws.hh
  ws.cc
  webserver.cc
  webserver.hh
  session.cc
  session.hh
  receiver.cc
  dynlistener.cc
  dynlistener.hh
  dynhandler.cc
  dynhandler.hh
  resolver.hh
  resolver.cc
  slavecommunicator.cc
  mastercommunicator.cc
  communicator.cc
  communicator.hh
  dnsproxy.cc
  dnsproxy.hh
  common_startup.cc
  common_startup.hh
  utility.hh
  iputils.hh
  unix_semaphore.cc
  sha.hh
  md5.hh
  lua-pdns.cc
  lua-auth.cc
  lua-auth.hh
  json.cc
  json.hh
  version.hh
  version.cc
  rfc2136handler.cc
  responsestats.cc
  responsestats.hh
)

add_executable(PDNS ${PDNS_SRCS})
set_target_properties(PDNS PROPERTIES OUTPUT_NAME "pdns_server")
install(TARGETS PDNS DESTINATION bin)
target_link_libraries(PDNS DNSUTILITY_LIB DNS_LIB BINDBACKEND_LIB ${FLEX_LIBRARIES} ${BISON_LIBRARIES} GSQLBACKEND_LIB PDNSAUTH_LIB)
target_link_libraries(PDNS polarssl)
target_link_libraries(PDNS ${Boost_LIBRARIES})
target_link_libraries(PDNS ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(PDNS ${DL_LIBRARY})
target_link_libraries(PDNS ${LUA_LIBRARIES})
message("PDNS linking with static backends: ${PDNS_STATIC_BACKENDS}")
target_link_libraries(PDNS ${PDNS_STATIC_BACKENDS})


set(PDNSSEC_SRCS
  pdnssec.cc
)
add_executable(pdnssec ${PDNSSEC_SRCS})
install(TARGETS pdnssec DESTINATION bin)
target_link_libraries(pdnssec DNSUTILITY_LIB DNS_LIB BINDBACKEND_LIB ${FLEX_LIBRARIES} ${BISON_LIBRARIES} GSQLBACKEND_LIB PDNSAUTH_LIB)
target_link_libraries(pdnssec polarssl)
target_link_libraries(pdnssec ${Boost_LIBRARIES})
target_link_libraries(pdnssec ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(pdnssec ${DL_LIBRARY})


set(PDNS_CONTROL_SRCS
  dynloader.cc dynmessenger.cc
)
add_executable(pdns_control ${PDNS_CONTROL_SRCS})
install(TARGETS pdns_control DESTINATION bin)
target_link_libraries(pdns_control DNSUTILITY_LIB)
target_link_libraries(pdns_control ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(pdns_control ${Boost_LIBRARIES})


###############################################################################
# Recursor
###############################################################################

set(REC_CONTROL_SRCS
  rec_control.cc
  rec_channel.cc rec_channel.hh
)
add_executable(rec_control ${REC_CONTROL_SRCS})
install(TARGETS rec_control DESTINATION bin)
target_link_libraries(rec_control DNSUTILITY_LIB)
target_link_libraries(rec_control ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(rec_control ${Boost_LIBRARIES})


set(PDNS_RECURSOR_SRCS
  syncres.cc syncres.hh
  resolver.hh
  lwres.cc pdns_recursor.cc reczones.cc lwres.hh
  dns.cc
  recursor_cache.cc recursor_cache.hh
  zoneparser-tng.cc zoneparser-tng.hh
  rec_channel.cc rec_channel.hh
  rec_channel_rec.cc
  mtasker.hh
  selectmplexer.cc epollmplexer.cc
  htimer.cc htimer.hh
  lua-pdns.cc lua-pdns.hh lua-recursor.cc lua-recursor.hh
  recpacketcache.cc recpacketcache.hh cachecleaner.hh
  json_ws.cc json_ws.hh
  json.cc json.hh
  version.cc version.hh
)
add_executable(pdns_recursor ${PDNS_RECURSOR_SRCS})
install(TARGETS pdns_recursor DESTINATION bin)
target_link_libraries(pdns_recursor DNSUTILITY_LIB DNS_LIB)
target_link_libraries(pdns_recursor ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(pdns_recursor ${Boost_LIBRARIES})
target_link_libraries(pdns_recursor ${LUA_LIBRARIES})


###############################################################################
# Tools
###############################################################################

macro(add_tool _tool_name)
  add_executable(${_tool_name} ${${_tool_name}_SRCS})
  install(TARGETS ${_tool_name} DESTINATION bin)
  target_link_libraries(${_tool_name} DNSUTILITY_LIB DNSPCAP_LIB DNS_LIB)
  target_link_libraries(${_tool_name} ${CMAKE_THREAD_LIBS_INIT} ${Boost_LIBRARIES})
endmacro(add_tool)


set(dnswasher_SRCS
  dnswasher.cc
)
add_tool(dnswasher)

set(dnsbulktest_SRCS
  dnsbulktest.cc
)
add_tool(dnsbulktest)

set(dnsscan_SRCS
  dnsscan.cc anadns.hh
)
add_tool(dnsscan)

set(dnsreplay_SRCS
  dnsreplay.cc anadns.hh
)
add_tool(dnsreplay)

set(dnsscope_SRCS
  dnsscope.cc
)
add_tool(dnsscope)

set(dnsgram_SRCS
  dnsgram.cc
)
add_tool(dnsgram)

set(nproxy_SRCS
  nproxy.cc
  selectmplexer.cc mplexer.hh
)
add_tool(nproxy)

set(dnsdemog_SRCS
  dnsdemog.cc
)
add_tool(dnsdemog)



