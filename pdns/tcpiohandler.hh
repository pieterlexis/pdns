
#pragma once

class TLSCtx
{
public:
  TLSCtx(const TLSFrontend& fe);
  virtual ~TLSCtx();
  virtual TLSConnection* getConnection(int socket);
};

class TLSConnection
{
  TLSConnection(int socket, std::shared_ptr<TLSCtx>): d_socket(socket)
  {
  }
#warning FIXME: deleted?
  virtual ~TLSConnection() = 0;
  virtual size_t read(void* buffer, size_t bufferSize, unsigned int readTimeout) = 0;
  virtual size_t write(const void* buffer, size_t bufferSize, unsigned int writeTimeout) = 0;
private:
  int d_socket{-1};
};
  
class TCPIOHandler
{
public:
  TCPIOHandler(int socket, std::shared_ptr<TLSCtx> ctx): d_socket(socket)
  {
    if (ctx) {
      d_conn = ctx->getConnection(socket);
    }
  }
  size_t read(void* buffer, size_t bufferSize, unsigned int readTimeout)
  {
    if (d_conn) {
      return d_conn->read(buffer, bufferSize, readTimeout);
    } else {
      return readn2WithTimeout(d_socket, buffer, bufferSize, readTimeout);
    }
  }
  size_t write(const void* buffer, size_t bufferSize, unsigned int writeTimeout)
  {
    if (d_conn) {
      return d_conn->write(buffer, bufferSize, writeTimeout);
    }
    else {
      return writen2WithTimeout(d_socket, buffer, bufferSize, writeTimeout);
    }
  }
private:
  std::unique_ptr<TLSConnection> d_conn{nullptr};
  int d_socket{-1};
};
