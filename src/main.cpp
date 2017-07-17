#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef USE_NET

#define BACKEND "net"
#include <experimental/net>
namespace net = std::experimental::net;

#elif USE_ASIO

#define BACKEND "asio"
#include <asio.hpp>
namespace net = asio;

#else
#error Missing backend selection.
#endif

constexpr std::size_t message_size = 1024 * 4;
constexpr std::size_t buffer_size  = 1024 * 8;

void report(std::exception_ptr e) {
  static std::mutex mutex;
  try {
    std::rethrow_exception(e);
  }
  catch (const std::system_error& e) {
    std::lock_guard<std::mutex> lock(mutex);
    std::cerr << "[" << e.code() << "] " << e.what() << std::endl;
  }
  catch (const std::exception& e) {
    std::lock_guard<std::mutex> lock(mutex);
    std::cerr << e.what() << std::endl;
  }
}

class session : public std::enable_shared_from_this<session> {
public:
  session(net::ip::tcp::socket socket) :
    socket_(std::move(socket))
  {}

  session(session&& other) = default;
  session(const session& other) = delete;

  session& operator=(session&& other) = default;
  session& operator=(const session& other) = delete;

  void start() {
    recv();
  }

  void send(const char* data, std::size_t size) {
    auto store = std::make_unique<char[]>(size);
    std::memcpy(store.get(), data, size);
    const auto store_data = store.get();
    send(store_data, size, [self = shared_from_this(), store = std::move(store)](const std::error_code& ec, std::size_t size) {
      if (ec && ec != net::error::operation_aborted && ec != net::error::connection_reset && ec != net::error::eof) {
        std::cerr << self.get() << ": send error: " << ec << ' ' << ec.message() << '\n';
      }
    });
  }

  template <typename Handler>
  void send(const char* data, std::size_t size, Handler&& handler) {
    net::async_write(socket_, net::buffer(data, size), std::forward<Handler>(handler));
  }

private:
  void recv() {
    const auto buffer = net::buffer(buffer_.data(), buffer_.size());
    socket_.async_read_some(buffer, [this, self = shared_from_this()](const std::error_code& ec, std::size_t size) {
      if (ec) {
        if (ec != net::error::operation_aborted && ec != net::error::connection_reset && ec != net::error::eof) {
          std::cerr << self.get() << ": recv error: " << ec << ' ' << ec.message() << '\n';
        }
        return;
      }
      send(buffer_.data(), size);
      recv();
    });
  }

  net::ip::tcp::socket socket_;
  std::array<char, buffer_size> buffer_;
};

class server {
public:
  server(net::io_context& io_context, const std::string& address, unsigned short port) :
    acceptor_(io_context, net::ip::tcp::endpoint(net::ip::make_address(address), port))
  {
    accept();
  }

  server(server&& other) = default;
  server(const server& other) = delete;

  server& operator=(server&& other) = default;
  server& operator=(const server& other) = delete;

private:
  void accept() {
    acceptor_.async_accept([this](const std::error_code& ec, net::ip::tcp::socket socket) {
      if (ec) {
        if (ec != net::error::operation_aborted) {
          std::cerr << "accept error: " << ec << ' ' << ec.message() << '\n';
        }
        return;
      }
      std::make_shared<session>(std::move(socket))->start();
      accept();
    });
  }

  net::ip::tcp::acceptor acceptor_;
};

class client {
public:
  client(const std::string& address, unsigned short port, std::size_t messages) :
    socket_(io_context_),
    work_(io_context_.get_executor()),
    endpoint_(net::ip::make_address(address), port),
    messages_(messages)
  {
    socket_.open(endpoint_.protocol());
    for (std::size_t i = 0; i < send_buffer_.size(); i++) {
      send_buffer_[i] = '0' + (i % 10);
    }
    std::error_code ec;
    socket_.connect(endpoint_, ec);
    if (ec) {
      throw std::system_error(ec, "connect");
    }
    thread_ = std::thread([this]() {
      try {
        io_context_.run();
      }
      catch (...) {
        report(std::current_exception());
      }
    });
  }

  client(client&& other) = delete;
  client(const client& other) = delete;

  client& operator=(client&& other) = delete;
  client& operator=(const client& other) = delete;

  void start() {
    recv(messages_ * send_buffer_.size());
    const auto buffer = net::buffer(send_buffer_);
    for (std::size_t i = 0; i < messages_; i++) {
      net::async_write(socket_, buffer, [](const std::error_code& ec, std::size_t size) {
        if (ec) {
          throw std::system_error(ec, "send");
        }
      });
    }
    work_.reset();
  }

  void join() {
    thread_.join();
  }

private:
  void recv(std::size_t read) {
    const auto buffer = net::buffer(recv_buffer_);
    socket_.async_read_some(buffer, [this, read](const std::error_code& ec, std::size_t size) {
      if (ec) {
        throw std::system_error(ec, "recv");
      }
      if (size < read) {
        recv(read - size);
      }
    });
  }

  std::thread thread_;
  net::io_context io_context_;
  net::executor_work_guard<net::io_context::executor_type> work_;
  net::ip::tcp::socket socket_;
  net::ip::tcp::endpoint endpoint_;
  std::size_t messages_ = 0;

  std::array<char, buffer_size> recv_buffer_;
  std::array<char, message_size> send_buffer_;
};

void test(std::size_t threads, std::size_t messages, const std::string& address, unsigned short port) {
  std::vector<std::unique_ptr<client>> clients;
  clients.resize(threads);

  for (auto& e : clients) {
    e = std::make_unique<client>(address, port, messages);
  }

  const auto beg = std::chrono::steady_clock::now();
  for (auto& e : clients) {
    e->start();
  }
  for (auto& e : clients) {
    e->join();
  }
  const auto end = std::chrono::steady_clock::now();

  const auto bytes = threads * messages * message_size * 2;  // sent + received
  const auto mib = (bytes / 1024.0 / 1024.0);
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - beg);
  const auto s = std::chrono::duration_cast<std::chrono::duration<double>>(end - beg);
  const auto mibps = mib / s.count();

  std::cout << std::fixed << std::setprecision(1)
    << mib << "MiB in " << ms.count() << "ms (" << mibps << " MiB/s)" << std::endl;
}

int main(int argc, char* argv[]) {
  try {
    const std::size_t tests       = argc > 1 ? std::stoull(argv[1]) : 5;
    const std::size_t threads     = argc > 2 ? std::stoull(argv[2]) : 20;
    const std::size_t messages    = argc > 3 ? std::stoull(argv[3]) : 4096;
    const std::string address     = argc > 4 ? argv[4] : "127.0.0.1";
    const auto port = static_cast<unsigned short>(argc > 5 ? std::stoul(argv[5]) : 8080);

    net::io_context io_context;
    server server(io_context, address, port);

    auto thread = std::thread([&]() {
      try {
        io_context.run();
      }
      catch (...) {
        report(std::current_exception());
      }
    });

    std::cout << "[" << BACKEND << "] " << address << ":" << port << " ("
      << threads << " threads, " << messages << " messages)" << std::endl;

    std::size_t width = 0;
    auto n = tests;
    do {
      ++width;
      n /= 10;
    } while (n);

    for (std::size_t i = 0; i < tests; i++) {
      std::cout << std::setw(width) << (i + 1) << "/" << tests << ": " << std::flush;
      test(threads, messages, address, port);
    }

    io_context.stop();
    thread.join();
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
#ifdef _MSC_VER
  if (IsDebuggerPresent()) {
    FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
    std::cout << "Press ENTER to continue . . ." << std::flush;
    std::cin.get();
  }
#endif
  return 0;
}
