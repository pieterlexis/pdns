#!/usr/bin/env python3

import requests
import sys


if len(sys.argv) < 2:
    print('Usage: {} PR_MUMBER...'.format(sys.argv[0]))
    sys.exit(1)

out = ''
for pr in sys.argv[1:]:
    if pr[0] == '#':
        pr = pr[1:]
    try:
        res = requests.get('https://api.github.com/repos/PowerDNS/pdns/pulls/'
                           '{}'.format(pr))
        pr_info = res.json()
    except (requests.exceptions.HTTPError, ValueError) as e:
        print(e)
        sys.exit(1)

    out += '-  `#{pr} <{url}>`__: {title}'.format(
        pr=pr, url=pr_info['html_url'], title=pr_info['title']
    )

    if pr_info['user']['login'].lower() not in ['ahupowerdns', 'habbie',
                                                'pieterlexis', 'rgacogne',
                                                'aerique']:
        try:
            user_info = requests.get(pr_info['user']['url']).json()
        except (requests.exceptions.HTTPError, ValueError) as e:
            print(e)
            sys.exit(1)
        out += ' ({})'.format(user_info['name'])

    out += '\n'

print(out)
