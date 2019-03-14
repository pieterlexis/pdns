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
#include "configparser.hh"

void Config::declareItem(const std::string &name, const ConfigItemType valType, const std::string &description, const std::string &help, const bool runtime, const bool isVector, const boost::any defaultValue) {
  if (d_configParsed) {
    throw std::runtime_error("Can not declare new items after the configuration has been parsed");
  }

  if (configElements.find(name) != configElements.end()) {
    throw std::runtime_error("Configuration item " + name + " is already declared");
  }

  if (name.empty()) {
    throw std::runtime_error("Configuration elements cannot be unnamed"); // FIXME should be asserted
  }

  if (help.empty()) {
    throw std::runtime_error("Configuration element '" + name + "' has no help"); // FIXME should be asserted
  }

  if (description.empty()) {
    throw std::runtime_error("Configuration element '" + name + "' has no description"); // FIXME should be asserted
  }

  if (defaultValue.empty()) {
    throw std::runtime_error("Configuration element '" + name + "' has no default value"); // FIXME should be asserted
  }

  try {
    if (isVector) {
      tryCastVector(defaultValue, valType);
    } else {
      tryCast(defaultValue, valType);
    }
  } catch(const std::runtime_error &e) {
    throw runtime_error("Default value for '" + name + "' is not of the right type: " + e.what());
  }

  configI c;
  c.t = valType;
  c.val = defaultValue;
  c.defaultValue = defaultValue;
  c.description = description;
  c.help = help;
  c.runtime = runtime;

  configElements[name] = c;
}

bool Config::getBool(const std::string &name) {
  isDeclared(name);
  if (configElements[name].t != ConfigItemType::Bool) {
    throw runtime_error("wrong type"); // FIXME
  }
  return boost::any_cast<bool>(configElements[name].val);
}

string Config::emitConfig() {
  // FIXME use YAML::Emitter
  return "";
}
