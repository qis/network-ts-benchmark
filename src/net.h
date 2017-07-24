#pragma once
#include <net>
#include <string>
#include <string_view>

class net_session {
public:
  net_session(std::net::ip::tcp::socket socket) :
    socket_(std::move(socket)) {}

  virtual ~net_session() = default;

  template <typename Handler>
  void recv(char* data, std::size_t size, Handler&& handler) {
    const auto buffer = std::net::buffer(data, size);
    socket_.async_read_some(buffer, std::forward<Handler>(handler));
  }

  template <typename Handler>
  void send(const char* data, std::size_t size, Handler&& handler) {
    std::net::async_write(socket_, std::net::buffer(data, size), std::forward<Handler>(handler));
  }

  static bool is_shutdown(const std::error_code& ec) {
    return ec == std::net::error::operation_aborted || ec == std::net::error::connection_reset || ec == std::net::error::eof;
  }

private:
  std::net::ip::tcp::socket socket_;
};

class net_server {
public:
  using io_context = std::net::io_context;
  using socket = std::net::ip::tcp::socket;
  using session = net_session;

  net_server(io_context& io_context, std::string_view address, std::string_view service) :
    acceptor_(io_context) {
    std::net::ip::tcp::resolver resolver(io_context);
    const auto it = resolver.resolve(std::string(address), std::string(service), std::net::ip::resolver_base::flags::numeric_host);
    const auto endpoint = it.begin()->endpoint();
    acceptor_.open(endpoint.protocol());
    acceptor_.bind(endpoint);
    acceptor_.listen();
  }

  template <typename Handler>
  void accept(Handler&& handler) {
    acceptor_.async_accept(std::forward<Handler>(handler));
  }

  std::net::ip::tcp::endpoint endpoint() const {
    return acceptor_.local_endpoint();
  }

  static bool is_shutdown(const std::error_code& ec) {
    return ec == std::net::error::operation_aborted;
  }

  static const char* type() {
    return "n";
  }

private:
  std::net::ip::tcp::acceptor acceptor_;
  std::exception_ptr exception_;
  std::thread thread_;
};

class net_client {
public:
  using io_context = std::net::io_context;
  using socket = std::net::ip::tcp::socket;

  net_client(io_context& io_context, std::string_view address, std::string_view service) :
    socket_(io_context) {
    std::net::ip::tcp::resolver resolver(io_context);
    const auto it = resolver.resolve(std::string(address), std::string(service));
    const auto endpoint = it.begin()->endpoint();
    socket_.open(endpoint_.protocol());
    socket_.connect(endpoint);
  }

  template <typename Handler>
  void recv(char* data, std::size_t size, Handler&& handler) {
    const auto buffer = std::net::buffer(data, size);
    socket_.async_read_some(buffer, std::forward<Handler>(handler));
  }

  template <typename Handler>
  void send(const char* data, std::size_t size, Handler&& handler) {
    std::net::async_write(socket_, std::net::buffer(data, size), std::forward<Handler>(handler));
  }

  static const char* type() {
    return "n";
  }

private:
  std::net::ip::tcp::socket socket_;
  std::net::ip::tcp::endpoint endpoint_;
};
