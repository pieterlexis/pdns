#!/usr/bin/env python

import os

from backend import BackendHandler
from pdns.remotebackend import PipeConnector


def main():
    path = os.path.dirname(os.path.realpath(__file__))
    connector = PipeConnector(
        BackendHandler, options={"dbpath": os.path.join(path, "remote.sqlite3"), "rawlog": "/tmp/raw.json"}
    )
    connector.run()


main()
