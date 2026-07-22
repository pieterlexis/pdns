import dns.message
import dns.flags
import dns.rcode
import dns.rdatatype
import dns.rrset


from authtests import AuthTest

aliasUDPReactorRunning = False


class TestDelegation(AuthTest):
    _config_template = """
launch={backend}
logging-structured
"""

    _zones = {
        "example.org": """
example.org.                 3600 IN SOA  {soa}
example.org.                 3600 IN NS   ns1.example.org.
example.org.                 3600 IN NS   ns2.example.org.
ns1.example.org.             3600 IN A    {prefix}.10
ns2.example.org.             3600 IN A    {prefix}.11

deleg-only.insecure.example.org.           3600 IN DELEG server-ipv4=192.0.2.1,192.0.2.2 server-ipv6=2001:db8:53::1,2001:db8:53::2 server-name=ns1.deleg-server-name.example.org,ns2.deleg-server-name.example.org

ns-only.insecure.example.org.         3600 IN NS ns1.ns-only.example.org.
ns-only.insecure.example.org.         3600 IN NS ns2.ns-only.example.org.
ns1.ns-only.insecure.example.org.     3600 IN A  {prefix}.10
ns2.ns-only.insecure.example.org.     3600 IN A  {prefix}.11

both.insecure.example.org.         3600 IN NS    ns1.both.example.org.
both.insecure.example.org.         3600 IN NS    ns2.both.example.org.
both.insecure.example.org.         3600 IN DELEG server-name=ns1.both.example.org,ns2.both.example.org
ns1.both.insecure.example.org.     3600 IN A  {prefix}.10
ns2.both.insecure.example.org.     3600 IN A  {prefix}.11

deleg-only.secure.example.org.           3600 IN DELEG server-ipv4=192.0.2.1,192.0.2.2 server-ipv6=2001:db8:53::1,2001:db8:53::2 server-name=ns1.deleg-server-name.example.org,ns2.deleg-server-name.example.org
deleg-only.secure.example.org.           3600 IN DS 44030 8 2 D4C3D5552B8679FAEEBC317E5F048B614B2E5F607DC57F1553182D49AB2179F7 ;; Fake DS, we only care that it exists

ns-only.secure.example.org.         3600 IN NS ns1.ns-only.example.org.
ns-only.secure.example.org.         3600 IN NS ns2.ns-only.example.org.
ns-only.secure.example.org          3600 IN DS 44030 8 2 D4C3D5552B8679FAEEBC317E5F048B614B2E5F607DC57F1553182D49AB2179F7 ;; Fake DS, we only care that it exists
ns1.ns-only.secure.example.org.     3600 IN A  {prefix}.10
ns2.ns-only.secure.example.org.     3600 IN A  {prefix}.11

both.secure.example.org.         3600 IN NS    ns1.both.example.org.
both.secure.example.org.         3600 IN NS    ns2.both.example.org.
both.secure.example.org.         3600 IN DELEG server-name=ns1.both.example.org,ns2.both.example.org
both.secure.example.org.         3600 IN DS 44030 8 2 D4C3D5552B8679FAEEBC317E5F048B614B2E5F607DC57F1553182D49AB2179F7 ;; Fake DS, we only care that it exists
ns1.both.secure.example.org.     3600 IN A  {prefix}.10
ns2.both.secure.example.org.     3600 IN A  {prefix}.11
        """,
    }

    def doQueries(self, edns=True, deFlag=False, dnssec=False):
        use_edns=0 if edns else None
        ednsflags=None
        if edns:
            ednsflags = dns.flags.DO if dnssec else 0
            ednsflags += 2**13 if deFlag else 0

        expected_rcode = dns.rcode.NOERROR if edns and deFlag else dns.rcode.NXDOMAIN

        query = dns.message.make_query("foo.deleg-only.insecure.example.org", dns.rdatatype.A, use_edns=use_edns, ednsflags=ednsflags)
        res = self.sendUDPQuery(query)
        self.assertRcodeEqual(res, expected_rcode)
        self.assertAnswerEmpty(res)

        query = dns.message.make_query("foo.deleg-only.secure.example.org", dns.rdatatype.A, use_edns=use_edns, ednsflags=ednsflags)
        res = self.sendUDPQuery(query)
        self.assertRcodeEqual(res, expected_rcode)
        self.assertAnswerEmpty(res)

        expected_rcode = dns.rcode.NOERROR
        query = dns.message.make_query("foo.ns-only.insecure.example.org", dns.rdatatype.A, use_edns=use_edns, ednsflags=ednsflags)
        res = self.sendUDPQuery(query)
        self.assertRcodeEqual(res, expected_rcode)
        self.assertAnswerEmpty(res)

        query = dns.message.make_query("foo.ns-only.secure.example.org", dns.rdatatype.A, use_edns=use_edns, ednsflags=ednsflags)
        res = self.sendUDPQuery(query)
        self.assertRcodeEqual(res, expected_rcode)
        self.assertAnswerEmpty(res)

        query = dns.message.make_query("foo.both.insecure.example.org", dns.rdatatype.A, use_edns=use_edns, ednsflags=ednsflags)
        res = self.sendUDPQuery(query)
        self.assertRcodeEqual(res, expected_rcode)
        self.assertAnswerEmpty(res)

        query = dns.message.make_query("foo.both.secure.example.org", dns.rdatatype.A, use_edns=use_edns, ednsflags=ednsflags)
        res = self.sendUDPQuery(query)
        self.assertRcodeEqual(res, expected_rcode)
        self.assertAnswerEmpty(res)

    def testNoEDNS(self):
        self.doQueries(False)

    def testDEFlag(self):
        self.doQueries(True, True, False)

    def testDOFlag(self):
        self.doQueries(True, False, True)

    def testDEPlusDOFlag(self):
        self.doQueries(True, True, True)
