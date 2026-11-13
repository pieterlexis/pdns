import dns.flags
import dns.message
import dns.name
import dns.rdataclass
import dns.rrset

from recursortests import RecursorTest


class DelegTest(RecursorTest):
    _confdir = "Deleg"
    _auth_zones = RecursorTest._default_auth_zones
    _config_template = """
    dnssec=validate
    """

    def testSecure(self):
        expected = dns.rrset.from_text("deleg.secure.example.", 0, dns.rdataclass.IN, "A", "192.0.2.112")
        query1 = dns.message.make_query("deleg.secure.example", "A", want_dnssec=True)
        query1.flags |= dns.flags.AD

        res = self.sendUDPQuery(query1)
        self.assertMessageIsAuthenticated(res)
        self.assertRRsetInAnswer(res, expected)
        self.assertMatchingRRSIGInAnswer(res, expected)
        self.assertAdditionalEmpty(res)
