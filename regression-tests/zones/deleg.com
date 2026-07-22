deleg.com. 3600 IN SOA  ns1.deleg.com. mail.deleg.com. (
				2026072201 ; serial
				14400      ; refresh (2 hours 30 minutes)
				3600       ; retry (7 minutes 30 seconds)
				604800     ; expire (1 week)
				3600       ; minimum (7 minutes 30 seconds)
			)
deleg.com.                 3600 IN NS   ns1.deleg.com.
deleg.com.                 3600 IN NS   ns2.deleg.com.
ns1.deleg.com.             3600 IN A    192.0.2.10
ns2.deleg.com.             3600 IN A    192.0.2.11

deleg-only.insecure.deleg.com.           3600 IN DELEG server-ipv4=192.0.2.1,192.0.2.2 server-ipv6=2001:db8:53::1,2001:db8:53::2 server-name=ns1.deleg-server-name.deleg.com,ns2.deleg-server-name.deleg.com

ns-only.insecure.deleg.com.         3600 IN NS ns1.ns-only.insecure.deleg.com.
ns-only.insecure.deleg.com.         3600 IN NS ns2.ns-only.insecure.deleg.com.
ns1.ns-only.insecure.deleg.com.     3600 IN A  192.0.2.10
ns2.ns-only.insecure.deleg.com.     3600 IN A  192.0.2.11

both.insecure.deleg.com.         3600 IN NS    ns1.both.insecure.deleg.com.
both.insecure.deleg.com.         3600 IN NS    ns2.both.insecure.deleg.com.
both.insecure.deleg.com.         3600 IN DELEG server-name=ns1.both.deleg.com,ns2.both.deleg.com
ns1.both.insecure.deleg.com.     3600 IN A  192.0.2.10
ns2.both.insecure.deleg.com.     3600 IN A  192.0.2.11

deleg-only.secure.deleg.com.           3600 IN DELEG server-ipv4=192.0.2.1,192.0.2.2 server-ipv6=2001:db8:53::1,2001:db8:53::2 server-name=ns1.deleg-server-name.deleg.com,ns2.deleg-server-name.deleg.com
deleg-only.secure.deleg.com.           3600 IN DS 44030 8 2 D4C3D5552B8679FAEEBC317E5F048B614B2E5F607DC57F1553182D49AB2179F7 ;; Fake DS, we only care that it exists

ns-only.secure.deleg.com.         3600 IN NS ns1.ns-only.secure.deleg.com.
ns-only.secure.deleg.com.         3600 IN NS ns2.ns-only.secure.deleg.com.
ns-only.secure.deleg.com.         3600 IN DS 44030 8 2 D4C3D5552B8679FAEEBC317E5F048B614B2E5F607DC57F1553182D49AB2179F7 ;; Fake DS, we only care that it exists
ns1.ns-only.secure.deleg.com.     3600 IN A  192.0.2.10
ns2.ns-only.secure.deleg.com.     3600 IN A  192.0.2.11

both.secure.deleg.com.         3600 IN NS    ns1.both.secure.deleg.com.
both.secure.deleg.com.         3600 IN NS    ns2.both.secure.deleg.com.
both.secure.deleg.com.         3600 IN DELEG server-name=ns1.both.deleg.com,ns2.both.deleg.com
both.secure.deleg.com.         3600 IN DS 44030 8 2 D4C3D5552B8679FAEEBC317E5F048B614B2E5F607DC57F1553182D49AB2179F7 ;; Fake DS, we only care that it exists
ns1.both.secure.deleg.com.     3600 IN A  192.0.2.10
ns2.both.secure.deleg.com.     3600 IN A  192.0.2.11
