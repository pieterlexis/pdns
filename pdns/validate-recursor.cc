#include "validate.hh"
#include "validate-recursor.hh"
#include "syncres.hh"
#include "logger.hh"

DNSSECValidationMode g_dnssecValidationMode{DNSSECValidationMode::Process};
DNSSECBogusServfailMode g_dnssecBogusServfailMode{DNSSECBogusServfailMode::Off};

bool checkDNSSECDisabled() {
  return warnIfDNSSECDisabled("");
}

bool warnIfDNSSECDisabled(const string& msg) {
  if(g_dnssecValidationMode == DNSSECValidationMode::Off) {
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
