from recursortests import RecursorTest
import dns

class TestWellKnown(RecursorTest):
    _auths_zones = None
    _confdir = 'WellKnown'
    _roothints = None
    _recursor_ports = {'validate': 5300}

    def testServFail(self):
        names = ['servfail.nl', 'dnssec-failed.org']
        results = []
        for name in names:
            query = dns.message.make_query(name, 'SOA')
            results.append(self.sendUDPQuery(query))

        self.assertEqual(len(results), len(names))

        for result in results:
            self.assertEqual(result.rcode(), dns.rcode.SERVFAIL)

    def testNoError(self):
        names = ['powerdns.com', 'nlnetlabs.nl', 'knot-dns.cz']
        results = []
        for name in names:
            query = dns.message.make_query(name, 'SOA')
            results.append(self.sendUDPQuery(query))

        self.assertEqual(len(results), len(names))

        for result in results:
            self.assertEqual(result.rcode(), dns.rcode.NOERROR)
