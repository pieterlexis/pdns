import dns
from recursortests import RecursorTest
from twisted.internet.protocol import DatagramProtocol
from twisted.internet import reactor
import os
import threading


class testNoRRSIG(RecursorTest):
    _confdir = 'NoRRSIG'
    _config_template = """dnssec=validate"""

    _rec_env = {'LD_PRELOAD': os.environ.get('LIBFAKETIME'),
                'FAKETIME': '2017-09-13 00:00:00'}

    @classmethod
    def setUp(cls):
        confdir = os.path.join('configs', cls._confdir)
        cls.wipeRecursorCache(confdir)

    def testNoRRSIGS(self):
        """
        Go Bogus if the DNSKEY does not have signatures
        """

        query = dns.message.make_query('record.norrsigs.example', 'A')
        query.flags |= dns.flags.AD

        res = self.sendUDPQuery(query)

        self.assertRcodeEqual(res, dns.rcode.SERVFAIL)

    def testNoRRSIGSonRecords(self):
        """
        Go Bogus if the DNSKEY does have signatures, but the records don't
        """

        query = dns.message.make_query('record-sig.norrsigs.example', 'A')
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
    got_dnskey = False
    def datagramReceived(self, datagram, address):
        request = dns.message.from_wire(datagram)

        response = dns.message.make_response(request)
        response.flags = dns.flags.AA + dns.flags.QR

        print request.question[0].name
        print request.question[0].rdtype

        if request.question[0].rdtype == dns.rdatatype.A and str(request.question[0].name) == 'record.norrsigs.example.':
            answer = dns.rrset.from_text('record.norrsigs.example.', 15, dns.rdataclass.IN, 'A', '127.0.0.1')
            response.answer.append(answer)
        if request.question[0].rdtype == dns.rdatatype.A and str(request.question[0].name) == 'record-sig.norrsigs.example.':
            answer = dns.rrset.from_text('record-sig.norrsigs.example.', 15, dns.rdataclass.IN, 'A', '127.0.0.1')
            response.answer.append(answer)
        if request.question[0].rdtype == dns.rdatatype.DNSKEY and str(request.question[0].name) == 'norrsigs.example.':
            answer = dns.rrset.from_text('norrsigs.example.', 15, dns.rdataclass.IN, 'DNSKEY', '257 3 13 hcrXrO3sQUacsA14tLuNoVEXI/x5saSaes/S26hrRbiz01suZDTcg6aokZZSpsXZlAdCmCFM9IazsQLYZS8Emg==')
            response.answer.append(answer)
            if not self.got_dnskey:
                self.got_dnskey = True
            else:
                answer = dns.rrset.from_text('norrsigs.example.', 15, dns.rdataclass.IN, 'RRSIG', 'DNSKEY 13 2 3600 20170921000000 20170831000000 63916 norrsigs.example. hMC8R6iVU4RDOi82UqIsp0PrFDQvHNDs/+kLtLkjPndw9YHDJC+06xRH ehaKf5xZNNnAxU67pYSnG/snIyNlfw==')
                response.answer.append(answer)

        self.transport.write(response.to_wire(), address)
