cmake_minimum_required(VERSION 2.6)

project(PowerDNS C CXX)
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

# Version
execute_process(COMMAND "date" "+%Y%m%d%H%M%S" OUTPUT_VARIABLE DATE_STAMP)
string(REGEX REPLACE "(.+)\r?\n" "\\1" DATE_STAMP ${DATE_STAMP})
execute_process(COMMAND git describe --match NONE --always OUTPUT_VARIABLE GIT_VERSION)
string(REGEX REPLACE "(.+)\r?\n" "\\1" GIT_VERSION ${GIT_VERSION})
if(NOT GIT_VERSION)
  set(GIT_VERSION "export")
else()
  set(GIT_VERSION "git-${GIT_VERSION}")
endif()
# If VERSION is given, use it.
if(NOT VERSION)
  set(VERSION ${GIT_VERSION})
endif()
message("${PROJECT_NAME} Version ${VERSION}")
set(DIST_HOST "")
set(BUILD_DATE ${DATE_STAMP})
set(BUILD_HOST "")

# Load feature finders
include(FindPkgConfig)
include(FeatureSummary)
include(CheckFunctionExists)

# Default to Release builds
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()

# Default to build shared libs
if(NOT DEFINED BUILD_SHARED_LIBS)
        set(BUILD_SHARED_LIBS ON)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/pdns)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/pdns/backends/bind)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/pdns/backends/gsql)

check_function_exists(strcasestr HAVE_STRCASESTR)

# pthreads
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

# boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.40 COMPONENTS program_options serialization REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

# libdl
find_library(DL_LIBRARY dl)

# Lua
pkg_search_module(LUA lua5.2 lua5.1 lua5.0)
if (LUA_FOUND)
  set(HAVE_LUA 1)
  include_directories(${LUA_INCLUDE_DIRS})
endif(LUA_FOUND)

# shipped libs
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/pdns/ext/rapidjson/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/pdns/ext/polarssl-1.1.2/include)

set(SYSCONFDIR "${CMAKE_INSTALL_PREFIX}/etc/powerdns")
set(LOCALSTATEDIR "${CMAKE_INSTALL_PREFIX}/share/powerdns")
set(POWERDNS_LIBRARY_PATH "${CMAKE_INSTALL_LIBDIR}/lib/powerdns/")

add_definitions("-DHAVE_CONFIG_H")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-cmake.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(pdns)
add_subdirectory(modules)
