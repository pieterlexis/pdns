DNSCrypt objects and functions
==============================

.. function:: addDNSCryptBind(address, provider, certificate, keyfile[, options])

  Adds a DNSCrypt listen socket on ``address``.

  :param string address: The address and port to listen on
  :param string provider: The provider name for this bind
  :param string certificate: Path to the certificate file
  :param string keyfile: Path to the key file of the certificate
  :param table options: A table with key: value pairs with options (see below)

  Options:

  * ``doTCP=true``: bool - Also bind on TCP on ``address``.
  * ``reusePort=false``: bool - Set the ``SO_REUSEPORT`` socket option.
  * ``tcpFastOpenSize=0``: int - Set the TCP Fast Open queue size, enabling TCP Fast Open when available and the value is larger than 0
  * ``interface=""``: str - Sets the network interface to use
  * ``cpus={}``: table - Set the CPU affinity for this listener thread, asking the scheduler to run it on a single CPU id, or a set of CPU ids. This parameter is only available if the OS provides the pthread_setaffinity_np() function.

.. function:: generateDNSCryptProviderKeys(publicKey, privateKey)

  Generate a new provider keypair and write them to ``publicKey`` and ``privateKey``.

  :param string publicKey: path to write the public key to
  :param string privateKey: path to write the private key to

.. function:: generateDNSCryptCertificate(privatekey, certificate, keyfile, serial, validFrom, validUntil, version)
  .. versionchanged:: 1.3.0
    ``version`` optional parameter added.

  generate a new resolver private key and related certificate, valid from the ``validFrom`` UNIX timestamp until the ``validUntil`` one, signed with the provider private key.

  :param string privatekey: Path to the private key of the provider
  :param string certificate: Path where to write the certificate file
  :param string keyfile: Path where to write the private key for the certificate
  :param int serial: The certificate's serial number
  :param int validFrom: Unix timestamp from when the certificate will be valid
  :param int validUntil: Unix timestamp until when the certificate will be valid
  :param DNSCryptExchangeVersion version: The exchange version to use. Possible values are ``DNSCryptExchangeVersion::VERSION1`` (default, X25519-XSalsa20Poly1305) and ``DNSCryptExchangeVersion::VERSION2`` (X25519-XChacha20Poly1305)

.. function:: printDNSCryptProviderFingerprint(keyfile)

  Display the fingerprint of the provided resolver public key

  :param string keyfile: Path to the key file

.. function:: showDNSCryptBinds()

  Display the currently configured DNSCrypt binds

.. function:: getDNSCryptBind(n) -> DNSCryptContext

  Return the :class:`DNSCryptContext` object corresponding to the bind ``n``.

Certificates
------------

.. class:: DNSCryptCert

  Represents a DNSCrypt certificate.

.. classmethod:: DNSCryptCert:getClientMagic() -> string

  Return this certificate's client magic value.

.. classmethod:: DNSCryptCert:getEsVersion() -> string

  Return the cryptographic construction to use with this certificate,.

.. classmethod:: DNSCryptCert:getMagic() -> string

  Return the certificate magic number.

.. classmethod:: DNSCryptCert:getProtocolMinorVersion() -> string

  Return this certificate's minor version.

.. classmethod:: DNSCryptCert:getResolverPublicKey() -> string

  Return the public key corresponding to this certificate.

.. classmethod:: DNSCryptCert:getSerial() -> int

  Return the certificate serial number.

.. classmethod:: DNSCryptCert:getSignature() -> string

  Return this certificate's signature.

.. classmethod:: DNSCryptCert:getTSEnd() -> int

  Return the date the certificate is valid from, as a Unix timestamp.

.. classmethod:: DNSCryptCert:getTSStart() -> int

  Return the date the certificate is valid until (inclusive), as a Unix timestamp

Certificate Pairs
-----------------

.. class:: DNSCryptCertificatePair

  Represents a pair of DNSCrypt certificate and associated key

.. classmethod:: DNSCryptCertificatePair:getCertificate() -> DNSCryptCert

  Return the certificate.

.. classmethod:: DNSCryptCertificatePair:isActive() -> bool

  Return whether this pair is active and will be advertised to clients.


Context
-------

.. class:: DNSCryptContext

  Represents a DNSCrypt content. Can be used to rotate certs.

.. classmethod:: DNSCryptContext:addNewCertificate(cert, key[, active])

  .. versionadded:: 1.3.0

  Add a new certificate to the the given context. Active certificates are advertised to
  clients, inactive ones are not.

  :param DNSCryptCert cert: The certificate to add to the context
  :param DNSCryptPrivateKey key: The private key corresponding to the certificate
  :param bool active: Whether the certificate should be advertised to clients. Default is true

.. classmethod:: DNSCryptContext:generateAndLoadInMemoryCertificate(keyfile, serial, begin, end)

  .. versionchanged:: 1.3.0
    ``version`` optional parameter added.

  Generate a new resolver key and the associated certificate in-memory, sign it with the provided provider key, and add it to the context

  :param string keyfile: Path to the provider key file to use
  :param int serial: The serial number of the certificate
  :param int begin: Unix timestamp from when the certificate is valid
  :param int end: Unix timestamp from until the certificate is valid
  :param DNSCryptExchangeVersion version: The exchange version to use. Possible values are ``DNSCryptExchangeVersion::VERSION1`` (default, X25519-XSalsa20Poly1305) and ``DNSCryptExchangeVersion::VERSION2`` (X25519-XChacha20Poly1305)

.. classmethod:: DNSCryptContext:getCertificatePair(index) -> DNSCryptCertificatePair

  .. versionadded:: 1.3.0

  Return the certificate pair with index `index`.

  :param int index: The index of the certificate, starting at 0

.. classmethod:: DNSCryptContext:getCertificatePair(index) -> table of DNSCryptCertificatePair

  .. versionadded:: 1.3.0

  Return a table of certificate pairs.

.. classmethod:: DNSCryptContext:getCurrentCertificate() -> DNSCryptCert

  .. deprecated:: 1.3.0

  Return the current certificate. Deprecated as of 1.3.0 since more than one active certificate
  is now supported. For compatibility, it will return the first active certificate.

.. classmethod:: DNSCryptContext:getOldCertificate() -> DNSCryptCert

  .. deprecated:: 1.3.0

  Return the previous certificate. Deprecated as of 1.3.0 since more than one inactive certificate
  is now supported. For compatibility, it will return the first inactive certificate.

.. classmethod:: DNSCryptContext:getProviderName() -> string

  Return the provider name

.. classmethod:: DNSCryptContext:hasOldCertificate() -> bool

  .. deprecated:: 1.3.0

  Whether or not the context has a previous certificate, from a certificate rotation. Since
  1.3.0 several certificates active and inactive certificates can be used at the same time,
  so this function is deprecated. In order to keep compatibility this function will simply
  return `true` if at least one inactive certificate is configured.

.. classmethod:: DNSCryptContext:loadNewCertificate(certificate, keyfile[, active])

  .. versionchanged:: 1.3.0
    ``active`` optional parameter added.

  Load a new certificate and the corresponding private key. If `active` is false, the
  certificate will not be advertised to clients but can still be used to answer queries
  tied to it.

  :param string certificate: Path to a certificate file
  :param string keyfile: Path to a the corresponding key file
  :param bool active: Whether the certificate should be marked as active. Default is true

.. classmethod:: DNSCryptContext:markActive(serial)

  .. versionadded:: 1.3.0

  Mark the certificate with serial `serial` as active, meaning it will be advertised to clients.

  :param int serial: The serial of the number to mark as active

.. classmethod:: DNSCryptContext:markInactive(serial)

  .. versionadded:: 1.3.0

  Mark the certificate with serial `serial` as inactive, meaning it will not be advertised
  to clients but can still be used to answer queries tied to this certificate.

  :param int serial: The serial of the number to mark as inactive

.. classmethod:: DNSCryptContext:printCertificates()

  .. versionadded:: 1.3.0

  Print all the certificates.

.. classmethod:: DNSCryptContext:removeInactiveCertificate(serial)

  .. versionadded:: 1.3.0

  Remove the certificate with serial `serial`. It will not be possible to answer queries tied
  to this certificate, so it should have been marked as inactive for a certain time before that.
  Active certificates should be marked as inactive before they can be removed.

  :param int serial: The serial of the number to remove

