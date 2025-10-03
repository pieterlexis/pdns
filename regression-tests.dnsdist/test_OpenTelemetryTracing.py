#!/usr/bin/env python

import test_Protobuf
import dns
import time


class DNSDistOpenTelemetryProtobufTest(test_Protobuf.DNSDistProtobufTest):
    def checkProtobufOpenTelemetryBase(self, msg):
        self.assertTrue(msg)
        self.assertTrue(msg.HasField("openTelemetry"))


class TestOpenTelemetryTracingYAML(DNSDistOpenTelemetryProtobufTest):
    _yaml_config_params = [
        "_testServerPort",
        "_protobufServerPort",
    ]
    _yaml_config_template = """---
logging:
  open_telemetry_tracing: true

backends:
  - 127.0.0.1:%s

remote_logging:
 protobuf_loggers:
   - name: pblog
     address: 127.0.0.1:%s

query_rules:
 - name: Enable tracing
   selector:
     type: All
   action:
     type: SetTrace
     value: true

response_rules:
 - name: Do PB logging
   selector:
     type: All
   action:
     type: RemoteLog
     logger_name: pblog
"""

    def testOpenTelemetryTracing(self):
        name = "query.ot.tests.powerdns.com."

        target = "target.ot.tests.powerdns.com."
        query = dns.message.make_query(name, "A", "IN")
        response = dns.message.make_response(query)

        rrset = dns.rrset.from_text(
            name, 3600, dns.rdataclass.IN, dns.rdatatype.CNAME, target
        )
        response.answer.append(rrset)

        rrset = dns.rrset.from_text(
            target, 3600, dns.rdataclass.IN, dns.rdatatype.A, "127.0.0.1"
        )
        response.answer.append(rrset)

        (receivedQuery, receivedResponse) = self.sendUDPQuery(query, response)
        self.assertTrue(receivedQuery)
        self.assertTrue(receivedResponse)
        receivedQuery.id = query.id
        self.assertEqual(query, receivedQuery)
        self.assertEqual(response, receivedResponse)

        if self._protobufQueue.empty():
            # let the protobuf messages the time to get there
            time.sleep(1)

        # check the protobuf message corresponding to the UDP query
        msg = self.getFirstProtobufMessage()
