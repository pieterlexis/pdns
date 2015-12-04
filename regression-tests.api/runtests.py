#!/usr/bin/env python
#
# Shell-script style.

import os
import requests
import shutil
import subprocess
import sys
import tempfile
import time

SQLITE_DB = 'pdns.sqlite3'
WEBPORT = '5556'
APIKEY = '1234567890abcdefghijklmnopq-key'

NAMED_CONF_TPL = """
# Generated by runtests.py
options { directory "../regression-tests/zones/"; };
zone "example.com" { type master; file "example.com"; };
zone "powerdnssec.org" { type master; file "powerdnssec.org"; };
zone "cryptokeys.org" { type master; file "cryptokeys.org"; };
"""

AUTH_CONF_TPL = """
# Generated by runtests.py
launch=gsqlite3
gsqlite3-dnssec=on
gsqlite3-database="""+SQLITE_DB+"""
module-dir=../regression-tests/modules
#
"""

ACL_LIST_TPL = """
# Generated by runtests.py
# local host
127.0.0.1
::1
"""

REC_EXAMPLE_COM_CONF_TPL = """
# Generated by runtests.py
auth-zones+=example.com=../regression-tests/zones/example.com
"""

REC_CONF_TPL = """
# Generated by runtests.py
auth-zones=
forward-zones=
forward-zones-recurse=
api-config-dir=%(conf_dir)s
include-dir=%(conf_dir)s
"""


def ensure_empty_dir(name):
    if os.path.exists(name):
        shutil.rmtree(name)
    os.mkdir(name)


wait = ('--wait' in sys.argv)
if wait:
    sys.argv.remove('--wait')

tests = [opt for opt in sys.argv if opt.startswith('--tests=')]
if tests:
    for opt in tests:
        sys.argv.remove(opt)
tests = [opt.split('=', 1)[1] for opt in tests]

daemon = (len(sys.argv) == 2) and sys.argv[1] or None
if daemon not in ('authoritative', 'recursor'):
    print "Usage: ./runtests (authoritative|recursor)"
    sys.exit(2)

daemon = sys.argv[1]

pdns_recursor = os.environ.get("PDNSRECURSOR", "../pdns/recursordist/pdns_recursor")

if daemon == 'authoritative':

    # Prepare sqlite DB with some zones.
    subprocess.check_call(["rm", "-f", SQLITE_DB])
    subprocess.check_call(["make", "-C", "../pdns", "zone2sql"])

    with open('../modules/gsqlite3backend/schema.sqlite3.sql', 'r') as schema_file:
        subprocess.check_call(["sqlite3", SQLITE_DB], stdin=schema_file)

    with open('named.conf', 'w') as named_conf:
        named_conf.write(NAMED_CONF_TPL)
    with tempfile.TemporaryFile() as tf:
        p = subprocess.Popen(["../pdns/zone2sql", "--transactions", "--gsqlite", "--named-conf=named.conf"], stdout=tf)
        p.communicate()
        if p.returncode != 0:
            raise Exception("zone2sql failed")
        tf.seek(0, os.SEEK_SET)  # rewind
        subprocess.check_call(["sqlite3", SQLITE_DB], stdin=tf)

    with open('pdns.conf', 'w') as named_conf:
        named_conf.write(AUTH_CONF_TPL)

    subprocess.check_call(["../pdns/pdnsutil", "--config-dir=.", "secure-zone", "powerdnssec.org"])
    local_port = 5300
    pdnscmd = ("../pdns/pdns_server --daemon=no --local-address=127.0.0.1 --local-port="+str(local_port)+" --socket-dir=./ --no-shuffle --dnsupdate=yes --cache-ttl=0 --config-dir=. --api=yes --webserver-port="+WEBPORT+" --webserver-address=127.0.0.1 --api-key="+APIKEY).split()

else:
    conf_dir = 'rec-conf.d'
    ensure_empty_dir(conf_dir)
    with open('acl.list', 'w') as acl_list:
        acl_list.write(ACL_LIST_TPL)
    with open('recursor.conf', 'w') as recursor_conf:
        recursor_conf.write(REC_CONF_TPL % locals())
    with open(conf_dir+'/example.com..conf', 'w') as conf_file:
        conf_file.write(REC_EXAMPLE_COM_CONF_TPL)

    local_port = 5555
    pdnscmd = (pdns_recursor + " --daemon=no --socket-dir=. --config-dir=. --allow-from-file=acl.list --local-address=127.0.0.1 --local-port="+str(local_port)+" --webserver=yes --webserver-port="+WEBPORT+" --webserver-address=127.0.0.1 --webserver-password=something --api-key="+APIKEY).split()


# Now run pdns and the tests.
print "Launching pdns..."
print ' '.join(pdnscmd)
pdns = subprocess.Popen(pdnscmd, close_fds=True)

print "Waiting for webserver port to become available..."
available = False
for try_number in range(0, 10):
    try:
        res = requests.get('http://127.0.0.1:%s/' % WEBPORT)
        available = True
        break
    except:
        time.sleep(0.5)

if not available:
    print "Webserver port not reachable after 10 tries, giving up."
    pdns.terminate()
    pdns.wait()
    sys.exit(2)

print "Doing DNS query to create statistic data..."
subprocess.check_call(["dig", "@127.0.0.1", "-p", str(local_port), "example.com"])

print "Running tests..."
rc = 0
test_env = {}
test_env.update(os.environ)
test_env.update({'WEBPORT': WEBPORT, 'APIKEY': APIKEY, 'DAEMON': daemon, 'SQLITE_DB': SQLITE_DB})

try:
    print ""
    p = subprocess.check_call(["nosetests", "--with-xunit"] + tests, env=test_env)
except subprocess.CalledProcessError as ex:
    rc = ex.returncode
finally:
    if wait:
        print "Waiting as requested, press ENTER to stop."
        raw_input()
    pdns.terminate()
    pdns.wait()

sys.exit(rc)
