#!/bin/sh

INFILE=$1
shift

PKG_CONFIG=$1
shift

sbindir=$1
shift

if [ -z "$sbindir" ]; then
  # not all params were provided
  exit 1
fi

sandboxing_opts=""

systemd_version="$(${PKG_CONFIG} --modversion systemd)"

if [ $systemd_version -ge 211 ]; then
  sandboxing_opts="RestrictAddressFamilies=AF_UNIX AF_INET AF_INET6"
fi

if [ $systemd_version -ge 214 ]; then
  sandboxing_opts="${sandboxing_opts}\nProtectHome=true"
  sandboxing_opts="${sandboxing_opts}\n# ProtectSystem=full will disallow write access to /etc and /usr, possibly\n# not being able to write slaved-zones into sqlite3 or zonefiles.\nProtectSystem=full"
fi

if [ $systemd_version -ge 231 ]; then
  sandboxing_opts="${sandboxing_opts}\nRestrictRealtime=true"
fi

if [ $systemd_version -ge 232 ]; then
  sandboxing_opts="${sandboxing_opts}\nProtectControlGroups=yes"
  sandboxing_opts="${sandboxing_opts}\nProtectKernelModules=yes"
  sandboxing_opts="${sandboxing_opts}\nProtectKernelTunables=yes"
fi

if [ $systemd_version -ge 233 ]; then
  sandboxing_opts="${sandboxing_opts}\nRestrictNamespaces=true"
fi

if [ $systemd_version -ge 235 ]; then
  sandboxing_opts="${sandboxing_opts}\nLockPersonality=true"
fi

sed -e "s;[@]sbindir[@];${sbindir};" -e "s;[@]sandboxingopts[@].*;${sandboxing_opts};" < $INFILE
