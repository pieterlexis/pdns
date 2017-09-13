import dns
from recursortests import RecursorTest
from twisted.internet.protocol import DatagramProtocol
from twisted.internet import reactor
import threading

class testNoRRSIG(RecursorTest):
    _confdir = 'NoRRSIG'

    _config_template = """dnssec=validate"""

    def testNoRRSIGS(self):
        """
        Go Bogus if the DNSKEY does not have signatures
        """

        query = dns.message.make_query('record.norrsigs.example', 'A')
        query.flags |= dns.flags.AD

        res = self.sendUDPQuery(query)

        self.assertRcodeEqual(res, dns.rcode.SERVFAIL)

    @classmethod
    def startResponders(cls):
        print("Launching responders..")

        address = cls._PREFIX + '.16'
        port = 53

        reactor.listenUDP(port, UDPResponder(), interface=address)

        cls._UDPResponder = threading.Thread(name='UDP Responder', target=reactor.run, args=(False,))
        cls._UDPResponder.setDaemon(True)
        cls._UDPResponder.start()

    @classmethod
    def tearDownResponders(cls):
        reactor.stop()

class UDPResponder(DatagramProtocol):
    def datagramReceived(self, datagram, address):
        request = dns.message.from_wire(datagram)

        response = dns.message.make_response(request)
        response.flags = dns.flags.AA + dns.flags.QR

        if request.question[0].rdtype == dns.rdatatype.A and str(request.question[0].name) == 'record.norrsigs.example.':
            answer = dns.rrset.from_text('record.norrsigs.example.', 15, dns.rdataclass.IN, 'A', '127.0.0.1')
            response.answer.append(answer)
        if request.question[0].rdtype == dns.rdatatype.DNSKEY and str(request.question[0].name) == 'norrsigs.example.':
            answer = dns.rrset.from_text('norrsigs.example.', 15, dns.rdataclass.IN, 'DNSKEY', '257 3 13 hcrXrO3sQUacsA14tLuNoVEXI/x5saSaes/S26hrRbiz01suZDTcg6aokZZSpsXZlAdCmCFM9IazsQLYZS8Emg==')
            response.answer.append(answer)

        self.transport.write(response.to_wire(), address)
