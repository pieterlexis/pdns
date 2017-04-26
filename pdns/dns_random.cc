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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <openssl/rand.h>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits>
#include <stdexcept>
#include <stdint.h>
#include "dns_random.hh"

using namespace std;

static bool g_initialized;

void dns_random_init(const char data[16])
{
	RAND_seed(data, 16);
	g_initialized = true;
}

unsigned int dns_random(unsigned int n)
{
  if(!g_initialized)
    abort();
  uint32_t out;
  if (RAND_pseudo_bytes(reinterpret_cast<unsigned char*>(&out), sizeof(out)) < 0)
	abort();
  return out % n;
}

#if 0
int main()
{
  dns_random_init("0123456789abcdef");

  for(int n = 0; n < 16; n++)
    cerr<<dns_random(16384)<<endl;
}
#endif
