PowerDNS Security Advisory 2017-07: Memory leak in DNSSEC parsing
=================================================================

-  CVE: CVE-2017-15094
-  Date: TBD
-  Credit: Nixu
-  Affects: PowerDNS Authoritative from 4.0.0 up to and including 4.0.4,
   PowerDNS Recursor from 4.0.0 up to and including 4.0.6
-  Not affected: PowerDNS Authoritative 4.0.5, PowerDNS Recursor 4.0.7
-  Severity: Low
-  Impact:  Denial of service
-  Exploit: This problem can be triggered by an authoritative server
   sending crafted DNSSEC keys to the Recursor, or by a user able to
   place crafted DNSSEC keys in the Authoritative server's database.
-  Risk of system compromise: No
-  Solution: Upgrade to a non-affected version

An issue has been found in the DNSSEC parsing code of PowerDNS Authoritative
and Recursor during a code audit by Nixu, leading to a memory leak when parsing
specially crafted DNSSEC RSA or ECDSA keys. This issue has been assigned
CVE-2017-15094.

PowerDNS Authoritative from 4.0.0 up to and including 4.0.4, as well as
PowerDNS Recursor from 4.0.0 up to and including 4.0.6 are affected.

For those unable to upgrade to a new version, a minimal patch is
`available <https://downloads.powerdns.com/patches/2017-07>`__

We would like to thank Nixu for finding and subsequently reporting
this issue.
