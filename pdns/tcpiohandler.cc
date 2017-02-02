
#include "config.h"
#include "iputils.hh"
#include "tcpiohandler.hh"
#include "dolog.hh"

#ifdef HAVE_DNS_OVER_TLS
#ifdef HAVE_OPENSSL
#warning with openssl!
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

class OpenSSLTLSConnection: public TLSConnection
{
public:
  OpenSSLTLSConnection(int socket, unsigned int timeout, SSL_CTX* tlsCtx)
  {
    d_socket = socket;
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
        handleIORequest(res, timeout);
      }
    }
    while (res < 0);

    if (res == 0) {
      throw std::runtime_error("Error accepting TLS connection");
    }
  }

  virtual ~OpenSSLTLSConnection() override
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
        throw std::runtime_error("Error reading from TLS connection");
      }
    }
    else if (error == SSL_ERROR_WANT_WRITE) {
      res = waitForRWData(d_socket, false, timeout, 0);
      if (res <= 0) {
        throw std::runtime_error("Error waiting to write to TLS connection");
      }
    }
    else {
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
  void close() override
  {
    if (d_conn) {
      SSL_shutdown(d_conn);
    }
  }
private:
  SSL* d_conn{nullptr};
};

class OpenSSLTLSIOCtx: public TLSCtx
{
public:
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

    if (s_users.fetch_add(1) == 0) {
      OPENSSL_config(NULL);
      ERR_load_crypto_strings();
      OpenSSL_add_ssl_algorithms();
    }

    d_tlsCtx = SSL_CTX_new(SSLv23_server_method());
    if (!d_tlsCtx) {
      ERR_print_errors_fp(stderr);
      throw std::runtime_error("Error creating TLS context on " + fe.d_addr.toStringWithPort());
    }

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
        throw std::runtime_error("Error creating TLS BIO for " + fe.d_addr.toStringWithPort());
      }

      if (BIO_read_filename(bio, fe.d_caFile.c_str()) <= 0) {
        BIO_free(bio);
        ERR_print_errors_fp(stderr);
        throw std::runtime_error("Error reading TLS chain from file " + fe.d_caFile + " for " + fe.d_addr.toStringWithPort());
      }

      X509* x509 = nullptr;
      while ((x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL)) != nullptr) {
        if (!SSL_CTX_add_extra_chain_cert(d_tlsCtx, x509)) {
          ERR_print_errors_fp(stderr);
          BIO_free(bio);
          X509_free(x509);
          BIO_free(bio);
          throw std::runtime_error("Error reading certificate from chain " + fe.d_caFile + " for " + fe.d_addr.toStringWithPort());
        }
      }
      BIO_free(bio);
    }
  }

  virtual ~OpenSSLTLSIOCtx() override
  {
    cerr<<"in "<<__func__<<endl;
    if (d_tlsCtx) {
      SSL_CTX_free(d_tlsCtx);
    }

    if (s_users.fetch_sub(1) == 1) {
      cerr<<"in "<<__func__<<", DESTROY"<<endl;
      ERR_remove_thread_state(0);
      ERR_free_strings();

      EVP_cleanup();

      CONF_modules_finish();
      CONF_modules_free();
      CONF_modules_unload(1);

      CRYPTO_cleanup_all_ex_data();
    } else {
      cerr<<"in "<<__func__<<", remaining.."<<endl;
    }
  }

  std::unique_ptr<TLSConnection> getConnection(int socket, unsigned int timeout) override
  {
    return std::unique_ptr<OpenSSLTLSConnection>(new OpenSSLTLSConnection(socket, timeout, d_tlsCtx));
  }
private:
  SSL_CTX* d_tlsCtx{nullptr};
  static std::atomic<uint64_t> s_users;
};

std::atomic<uint64_t> OpenSSLTLSIOCtx::s_users(0);

#endif /* HAVE_OPENSSL */

#ifdef HAVE_S2N
#warning with s2n!
#include <s2n.h>
#include <fstream>

class S2NTLSConnection: public TLSConnection
{
public:
  S2NTLSConnection(int socket, unsigned int timeout, struct s2n_config* tlsCtx)
  {
    d_socket = socket;

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

      if (res < 0 && !status) {
        int savedErrno = s2n_errno;
        vinfolog("Error accepting TLS connection: %s (%s)", s2n_strerror(savedErrno, "EN"), s2n_connection_get_alert(d_conn));
        throw std::runtime_error("Error accepting TLS connection");
      }
      if (status) {
        handleIORequest(status, timeout);
      }
    }
    while (status);
  }

  virtual ~S2NTLSConnection() override
  {
    if (d_conn) {
      s2n_connection_free(d_conn);
    }
  }

  void handleIORequest(s2n_blocked_status status, unsigned int timeout)
  {
    int res = 0;
    if (status == S2N_BLOCKED_ON_READ) {
      res = waitForData(d_socket, timeout);
      if (res <= 0) {
        throw std::runtime_error("Error reading from TLS connection");
      }
    }
    else if (status == S2N_BLOCKED_ON_WRITE) {
      res = waitForRWData(d_socket, false, timeout, 0);
      if (res <= 0) {
        throw std::runtime_error("Error waiting to write to TLS connection");
      }
    }
    else {
      throw std::runtime_error("Error writing to TLS connection");
    }
  }

  size_t read(void* buffer, size_t bufferSize, unsigned int readTimeout) override
  {
    size_t got = 0;
    s2n_blocked_status status;
    do {
      ssize_t res = s2n_recv(d_conn, ((char *)buffer) + got, bufferSize - got, &status);
      if (res == 0) {
        throw std::runtime_error("Error reading from TLS connection");
      }
      else if (res > 0) {
        got += (size_t) res;
      }
      else if (res < 0 && !status) {
        throw std::runtime_error("Error reading from TLS connection");
      }
      if (got < bufferSize && status) {
        handleIORequest(status, readTimeout);
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
      if (res == 0) {
        throw std::runtime_error("Error writing to TLS connection");
      }
      else if (res > 0) {
        got += (size_t) res;
      }
      else if (res < 0 && !status) {
        throw std::runtime_error("Error writing to TLS connection");
      }
      if (got < bufferSize && status) {
        handleIORequest(status, writeTimeout);
      }
    }
    while (got < bufferSize);

    return got;
  }

  void close() override
  {
    if (d_conn) {
      s2n_blocked_status status;
      s2n_shutdown(d_conn, &status);
    }
  }

private:
  struct s2n_connection *d_conn{nullptr};
};

class S2NTLSIOCtx: public TLSCtx
{
public:
  S2NTLSIOCtx(const TLSFrontend& fe)
  {
    if (s_users.fetch_add(1) == 0) {
      s2n_init();
    }

    d_tlsCtx = s2n_config_new();
    if (!d_tlsCtx) {
      throw std::runtime_error("Error creating TLS context on " + fe.d_addr.toStringWithPort());
    }

    std::ifstream certStream(fe.d_certFile);
    std::string certContent((std::istreambuf_iterator<char>(certStream)),
                            (std::istreambuf_iterator<char>()));
    std::ifstream keyStream(fe.d_keyFile);
    std::string keyContent((std::istreambuf_iterator<char>(keyStream)),
                           (std::istreambuf_iterator<char>()));

    if (s2n_config_add_cert_chain_and_key(d_tlsCtx, certContent.c_str(), keyContent.c_str()) < 0) {
      s2n_config_free(d_tlsCtx);
      throw std::runtime_error("Error assigning certificate (from " + fe.d_certFile + ") and key (from " + fe.d_keyFile + ")");
    }
    certContent.clear();
    keyContent.clear();

    if (!fe.d_ciphers.empty()) {
      // ignored for now
    }
  }

  virtual ~S2NTLSIOCtx() override
  {
    if (d_tlsCtx) {
      s2n_config_free(d_tlsCtx);
    }

    if (s_users.fetch_sub(1) == 1) {
      s2n_cleanup();
    }
  }

  std::unique_ptr<TLSConnection> getConnection(int socket, unsigned int timeout) override
  {
    return std::unique_ptr<S2NTLSConnection>(new S2NTLSConnection(socket, timeout, d_tlsCtx));
  }

private:
  struct s2n_config* d_tlsCtx{nullptr};
  static std::atomic<uint64_t> s_users;
};

std::atomic<uint64_t> S2NTLSIOCtx::s_users(0);
#endif /* HAVE_S2N */

#endif /* HAVE_DNS_OVER_TLS */

bool TLSFrontend::setupTLS()
{
#ifdef HAVE_DNS_OVER_TLS
  /* get the "best" available provider */
  if (!d_provider.empty()) {
#ifdef HAVE_S2N
    if (d_provider == "s2n") {
      d_ctx = std::make_shared<S2NTLSIOCtx>(*this);
      return true;
    }
#endif /* HAVE_S2N */
#ifdef HAVE_OPENSSL
    if (d_provider == "openssl") {
      d_ctx = std::make_shared<OpenSSLTLSIOCtx>(*this);
      return true;
    }
#endif /* HAVE_OPENSSL */
  }
#ifdef HAVE_S2N
  d_ctx = std::make_shared<S2NTLSIOCtx>(*this);
#else /* HAVE_S2N */
#ifdef HAVE_OPENSSL
  d_ctx = std::make_shared<OpenSSLTLSIOCtx>(*this);
#endif /* HAVE_OPENSSL */
#endif /* HAVE_S2N */

#endif /* HAVE_DNS_OVER_TLS */
  return true;
}
