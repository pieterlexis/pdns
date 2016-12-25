
#include "config.h"
#include "tcpiohandler.hh"

#ifdef HAVE_DNS_OVER_TLS

#if HAVE_OPENSSL
class OpenSSLTLSConnection: public TLSConnection
{
public:
  OpenSSLTLSConnection(int socket, std::shared_ptr<OpenSSLTLSIOCtx> tlsCtx): d_socket(socket)
  {
    d_conn = SSL_new(tlsCtx);
    if (!d_conn) {
      vinfolog("Error creating TLS object");
      if (g_verbose) {
        ERR_print_errors_fp(stderr);
      }
      throw std::runtime_error("Error creating TLS object");
    }
    if (!SSL_set_fd(d_conn, d_socket)) {
      throw std::runtime_error("Error assigning socket");
    }
    int res = 0;
    do {
      res = SSL_accept(d_conn);
      if (res < 0) {
        handleIORequest(res);
      }
    }
    while (res < 0);

    if (res == 0) {
      throw std::runtime_error("Error accepting TLS connection");
    }
  }

  ~TLSIOHandler() override
  {
    if (d_conn) {
      SSL_free(d_conn);
    }
  }

  void handleIORequest(int res, unsigned int timeout)
  {
    int error = SSL_get_error(d_conn, res);
    if (error == SSL_ERROR_WANT_READ) {
      res = waitForData(d_socket, timeout);
      if (res <= 0) {
//        cerr<<"Error reading from TLS connection"<<endl;
        throw std::runtime_error("Error reading from TLS connection");
      }
    }
    else if (error == SSL_ERROR_WANT_WRITE) {
      res = waitForRWData(d_socket, false, timeout, 0);
      if (res <= 0) {
//        cerr<<"Error waiting to write to TLS connection"<<endl;
        throw std::runtime_error("Error waiting to write to TLS connection");
      }
    }
    else {
//      cerr<<"Error writing to TLS connection"<<endl;
      throw std::runtime_error("Error writing to TLS connection");
    }
  }

  size_t read(void* buffer, size_t bufferSize, unsigned int readTimeout) override
  {
    size_t got = 0;
    do {
      int res = SSL_read(d_conn, ((char *)buffer) + got, (int) (bufferSize - got));
      if (res == 0) {
        throw std::runtime_error("Error reading from TLS connection");
      }
      else if (res < 0) {
        handleIORequest(res, readTimeout);
      }
      else {
        got += (size_t) res;
      }
    }
    while (got < bufferSize);

    return got;
  }

  size_t write(const void* buffer, size_t bufferSize, unsigned int writeTimeout) override
  {
    size_t got = 0;
    do {
      int res = SSL_write(d_conn, ((char *)buffer) + got, (int) (bufferSize - got));
      if (res == 0) {
        throw std::runtime_error("Error writing to TLS connection");
      }
      else if (res < 0) {
        handleIORequest(res, writeTimeout);
      }
      else {
        got += (size_t) res;
      }
    }
    while (got < bufferSize);

    return got;
  }
private:
  SSL* d_conn{nullptr};
};

class OpenSSLTLSIOCtx: public TLSCtx
{
  OpenSSLTLSIOCtx(const TLSFrontend& fe) 
  {
    static const int sslOptions =
      SSL_OP_NO_SSLv2 |
      SSL_OP_NO_SSLv3 |
      SSL_OP_NO_COMPRESSION |
      SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION |
      SSL_OP_SINGLE_DH_USE |
      SSL_OP_SINGLE_ECDH_USE |
      SSL_OP_CIPHER_SERVER_PREFERENCE;

    d_tlsCtx = SSL_CTX_new(SSLv23_server_method());
    if (!d_tlsCtx) {
      ERR_print_errors_fp(stderr);
      throw std::runtime_exception("Error creating TLS context on %s!", fe.d_addr.toStringWithPort());
    }

    vinfolog("TLS CTX created for cs");
    SSL_CTX_set_options(d_tlsCtx, sslOptions);
    SSL_CTX_set_ecdh_auto(d_tlsCtx, 1);
    SSL_CTX_use_certificate_file(d_tlsCtx, fe.d_certFile.c_str(), SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(d_tlsCtx, fe.d_keyFile.c_str(), SSL_FILETYPE_PEM);

    if (!fe.d_ciphers.empty()) {
      SSL_CTX_set_cipher_list(d_tlsCtx, fe.d_ciphers.c_str());
    }

    if (!fe.d_caFile.empty()) {
      BIO *bio = BIO_new(BIO_s_file_internal());
      if (!bio) {
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Error creating TLS BIO for %s!", fe.d_addr.toStringWithPort());
      }

      if (BIO_read_filename(bio, fe.d_caFile.c_str()) <= 0) {
        BIO_free(bio);
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Error reading TLS chain from file %s for %s!", fe.d_caFile, fe.d_addr.toStringWithPort());
      }

      X509* x509 = nullptr;
      while ((x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL)) != nullptr) {
        if (!SSL_CTX_add_extra_chain_cert(d_tlsCtx, x509)) {
          ERR_print_errors_fp(stderr);
          BIO_free(bio);
          X509_free(x509);
          BIO_free(bio);
          throw std::runtime_error("Error reading certificate from chain %s for %s!", fe.d_caFile, fe.d_addr.toStringWithPort());
        }
      }
      BIO_free(bio);
    }
  }

  ~OpenSSLTLSCtx() 
  {
    if (d_tlsCtx) {
      SSL_CTX_free(d_tlsCtx);
    }
  }

  TCPIOHandler* getConnection() override
  {
    return new OpenSSLTLSConnection(d_tlsCtx);    
  }
private:
  SSL_CTX* d_tlsCtx{nullptr};
};
  
#endif


#ifdef HAVE_S2N
class S2NTLSConnection: public TLSConnection
{
public:
  S2NTLSConnection(int socket, std::shared_ptr<S2NTLSIOCtx> tlsCtx): d_socket(socket)
  {
    d_conn = s2n_connection_new(S2N_SERVER);
    if (!d_conn) {
      vinfolog("Error creating TLS object");
      throw std::runtime_error("Error creating TLS object");
    }
    if (s2n_connection_set_config(d_conn, tlsCtx) < 0) {
      throw std::runtime_error("Error assigning configuration");
    }
    if (s2n_connection_set_fd(d_conn, d_socket) < 0) {
      throw std::runtime_error("Error assigning socket");
    }
    s2n_blocked_status status;
    int res = 0;
    do {
      res = s2n_negotiate(d_conn, &status);
      if (res < 0) {
        throw std::runtime_error("Error accepting TLS connection");
      }
      if (status) {
        handleIORequest(status);
      }
    }
    while (status);
  }

  ~TLSIOHandler() override
  {
    if (d_conn) {
      s2n_connection_wipe(d_conn);
    }
  }

  void handleIORequest(s2n_blocked_status status, unsigned int timeout)
  {
    if (status == S2N_BLOCKED_ON_READ) {
      res = waitForData(d_socket, timeout);
      if (res <= 0) {
        cerr<<"Error reading from TLS connection"<<endl;
        throw std::runtime_error("Error reading from TLS connection");
      }
    }
    else if (status == S2N_BLOCKED_ON_WRITE) {
      res = waitForRWData(d_socket, false, timeout, 0);
      if (res <= 0) {
        cerr<<"Error waiting to write to TLS connection"<<endl;
        throw std::runtime_error("Error waiting to write to TLS connection");
      }
    }
    else {
      cerr<<"Error writing to TLS connection"<<endl;
      throw std::runtime_error("Error writing to TLS connection");
    }
  }

  size_t read(void* buffer, size_t bufferSize, unsigned int readTimeout) override
  {
    size_t got = 0;
    s2n_blocked_status status;
    do {
      ssize_t res = s2n_recv(d_conn, ((char *)buffer) + got, bufferSize - got, &status);
      if (res <= 0) {
        throw std::runtime_error("Error reading from TLS connection");
      }
      got += (size_t) res;
      if (status) {
        handeIORequest(status, readtimeout);
      }
    }
    while (got < bufferSize);

    return got;
  }

  size_t write(const void* buffer, size_t bufferSize, unsigned int writeTimeout) override
  {
    size_t got = 0;
    s2n_blocked_status status;
    do {
      ssize_t res = s2n_send(d_conn, ((char *)buffer) + got, bufferSize - got, &status);
      if (res <= 0) {
        throw std::runtime_error("Error writing to TLS connection");
      }
      got += (size_t) res;
      if (status) {
        handleIORequest(status, writeTimeout);
      }
    }
    while (got < bufferSize);

    return got;
  }
private:
  struct s2n_connection *d_tls{nullptr}; 
};

class S2NTLSIOCtx: public TLSCtx
{
  S2NSSLTLSIOCtx(const TLSFrontend& fe) 
  {
    d_tlsCtx = s2n_config_new();
    if (!d_tlsCtx) {
      throw std::runtime_error("Error creating TLS context on " + fe.d_addr.toStringWithPort());
    return false;
  }

  vinfolog("TLS CTX created for cs");
  s2n_config_add_cert_chain_and_key(d_tlsCtx, fe.d_cert, fe.d_key);

  if (!fe.d_ciphers.empty()) {
    // ignored for now
  }

  ~S2NTLSCtx()
  {
    if (d_tlsCtx) {
      #warning FIXME
    }
  }

  TCPIOHandler* getConnection() override
  {
    return new S2NTLSConnection(d_tlsCtx);    
  }
private:
   struct s2n_config* d_tlsCtx{nullptr};
};

bool TLSFrontend::setupTLS()
{
  /* get the "best" available provider */
  if (!d_provider.empty()) {
#ifdef HAVE_S2N
    if (d_provider == "s2n") {
      d_ctx = S2NTLSIOCtx(this);
      return true;
    }
#endif
#ifdef HAVE_OPENSSL
    if (d_provider == "openssl") {
      d_ctx = OpenSSLTLSIOCtx(this);
      return true;
    }
#endif
  }
#ifdef HAVE_S2N
  d_ctx = S2NTLSIOCtx(this);
#else
#ifdef HAVE_OPENSSL
  d_ctx = OpenSSLTLSIOCtx(this);
#endif
#endif
  return true;
}

#endif /* HAVE_DNS_OVER_TLS */
