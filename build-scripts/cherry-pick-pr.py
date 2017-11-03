#!/usr/bin/env python3

import requests
import sys
import subprocess


try:
    res = requests.get('https://api.github.com/repos/PowerDNS/pdns/pulls/'
                       '{}/commits'.format(sys.argv[1]))
except requests.exceptions.HTTPError as e:
    print(e)
    sys.exit(1)

try:
    json = res.json()
except ValueError as e:
    print(e)
    sys.exit(1)

commits = [c['sha'] for c in json]

try:
    command = ['git', 'cherry-pick', '-x', '--allow-empty'] + commits
    subprocess.check_call(command)
except subprocess.CalledProcessError as e:
    print(e)
    sys.exit(1)
