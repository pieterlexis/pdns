#!/bin/sh

if [ -z "${1}" ]; then
  exit 1
fi

if [ -z "${2}" ]; then
  exit 1
fi

rm -f "${2}"
echo 'R"FFIContent(' > "${2}"
cat "${1}" >> "${2}"
echo ')FFIContent"' >> "${2}"
