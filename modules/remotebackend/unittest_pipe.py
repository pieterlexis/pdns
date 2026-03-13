#!/usr/bin/env python3

from pdns.remotebackend import PipeConnector
from pdns_unittest import Handler

connector = PipeConnector(Handler)
connector.run()
