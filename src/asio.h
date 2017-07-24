#pragma once
#include <asio.hpp>
#include <string>
#include <string_view>

class asio_session {
public:
  asio_session(asio::ip::tcp::socket socket) :
    socket_(std::move(socket))
  {}

  virtual ~asio_session() = default;

  template <typename Handler>
  void recv(char* data, std::size_t size, Handler&& handler) {
    const auto buffer = asio::buffer(data, size);
    socket_.async_read_some(buffer, std::forward<Handler>(handler));
  }

  template <typename Handler>
  void send(const char* data, std::size_t size, Handler&& handler) {
    asio::async_write(socket_, asio::buffer(data, size), std::forward<Handler>(handler));
  }

  static bool is_shutdown(const std::error_code& ec) {
    return ec == asio::error::operation_aborted || ec == asio::error::connection_reset || ec == asio::error::eof;
  }

private:
  asio::ip::tcp::socket socket_;
};

class asio_server {
public:
  using io_context = asio::io_context;
  using socket = asio::ip::tcp::socket;
  using session = asio_session;

  asio_server(io_context& io_context, std::string_view address, std::string_view service) :
    acceptor_(io_context)
  {
    asio::ip::tcp::resolver resolver(io_context);
    const auto it = resolver.resolve(std::string(address), std::string(service), asio::ip::resolver_base::flags::numeric_host);
    const auto endpoint = it.begin()->endpoint();
    acceptor_.open(endpoint.protocol());
    acceptor_.bind(endpoint);
    acceptor_.listen();
  }

  template <typename Handler>
  void accept(Handler&& handler) {
    acceptor_.async_accept(std::forward<Handler>(handler));
  }

  asio::ip::tcp::endpoint endpoint() const {
    return acceptor_.local_endpoint();
  }

  static bool is_shutdown(const std::error_code& ec) {
    return ec == asio::error::operation_aborted;
  }

  static const char* type() {
    return "a";
  }

private:
  asio::ip::tcp::acceptor acceptor_;
  std::exception_ptr exception_;
  std::thread thread_;
};

class asio_client {
public:
  using io_context = asio::io_context;
  using socket = asio::ip::tcp::socket;

  asio_client(io_context& io_context, std::string_view address, std::string_view service) :
    socket_(io_context)
  {
    asio::ip::tcp::resolver resolver(io_context);
    const auto it = resolver.resolve(std::string(address), std::string(service));
    const auto endpoint = it.begin()->endpoint();
    socket_.open(endpoint_.protocol());
    socket_.connect(endpoint);
  }

  template <typename Handler>
  void recv(char* data, std::size_t size, Handler&& handler) {
    const auto buffer = asio::buffer(data, size);
    socket_.async_read_some(buffer, std::forward<Handler>(handler));
  }

  template <typename Handler>
  void send(const char* data, std::size_t size, Handler&& handler) {
    asio::async_write(socket_, asio::buffer(data, size), std::forward<Handler>(handler));
  }

  static const char* type() {
    return "a";
  }

private:
  asio::ip::tcp::socket socket_;
  asio::ip::tcp::endpoint endpoint_;
};
