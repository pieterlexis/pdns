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
#include "validate.hh"
#include "validate-recursor.hh"
#include "syncres.hh"
#include "logger.hh"

DNSSECMode g_dnssecmode{DNSSECMode::ProcessNoValidate};
bool g_dnssecLogBogus;

bool checkDNSSECDisabled() {
  return warnIfDNSSECDisabled("");
}

bool warnIfDNSSECDisabled(const string& msg) {
  if(g_dnssecmode == DNSSECMode::Off) {
    if (!msg.empty())
      L<<Logger::Warning<<msg<<endl;
    return true;
  }
  return false;
}

vState increaseDNSSECStateCounter(const vState& state)
{
  g_stats.dnssecResults[state]++;
  return state;
}
