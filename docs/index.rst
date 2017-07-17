PowerDNS Authoritative Nameserver
=================================

The PowerDNS Authoritative Server is a versatile nameserver which
supports a large number of backends. These backends can either be plain
zone files or be more dynamic in nature.

Examples of backends include relational databases, other DNS data
formats and coprocesses.

Backends
--------

PowerDNS has the concepts of 'backends'. A backend is a datastore that
the server will consult that contains DNS records (and some meta-data).
The backends range from database backends (Mysql, PostgreSQL, Oracle)
and Bind-zonefiles to co-processes and JSON API's.

Multiple backends can be enabled in the configuration by using the
:ref:`setting-launch` option. Each backend can be configured separately.

See the :doc:`backend <backends/index>` documentation for more information.

Getting Started
---------------

