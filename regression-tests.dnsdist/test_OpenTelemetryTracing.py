#!/usr/bin/env python

import dns
import time

import opentelemetry.proto.trace.v1.trace_pb2
import google.protobuf.json_format

import test_Protobuf


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
  - address: 127.0.0.1:%s
    protocol: Do53

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

        self.assertTrue(msg.HasField("openTelemetryTraceID"))
        self.assertTrue(msg.openTelemetryTraceID != "")

        self.assertTrue(msg.HasField("openTelemetryData"))
        data = opentelemetry.proto.trace.v1.trace_pb2.TracesData()
        data.ParseFromString(msg.openTelemetryData)
        json_data = google.protobuf.json_format.MessageToDict(
            data, preserving_proto_field_name=True
        )

        self.assertEqual(len(json_data["resource_spans"]), 1)
        self.assertEqual(
            len(json_data["resource_spans"][0]["resource"]["attributes"]), 4
        )

        # Ensure all attributes exist
        for field in json_data["resource_spans"][0]["resource"]["attributes"]:
            self.assertTrue(
                field["key"]
                in ["service.name", "query.qname", "query.qtype", "query.remote"]
            )

        # Ensure the values are correct
        # TODO: query.remote with port
        for k, v in {
            "service.name": "dnsdist",
            "query.qname": "query.ot.tests.powerdns.com",
            "query.qtype": "A",
        }.items():
            self.assertEqual(json_data["resource_spans"][0]["resource"]["attributes"])
