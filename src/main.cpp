#include "asio.h"
#include "net.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
# include <windows.h>
#elif defined(__linux__)
# include <sched.h>
#elif defined(__FreeBSD__)
# include <pthread_np.h>
#endif

namespace test {

using namespace std::chrono_literals;

using clock = std::chrono::steady_clock;

void usage() {
  std::ostringstream oss;
  oss << "usage: ntsb a|as|n|ns connections messages bytes address port\n\n"
    "  a|n          : server backend: asio | net\n"
    "  a|n          : client backend: asio | net\n"
    "  connections  : number of simultaneous connections  (default: 20)\n"
    "  messages     : number of messages per connection (default: 4096)\n"
    "  bytes        : message size in bytes (default: 4096)\n"
    "  address      : client/server address (default: 127.0.0.1)\n"
    "  service      : client/server service (default: 9000)\n"
    "\n"
    "Possible server/client combinations:\n"
    "  a  a,  a  n,  n  a,  n  n\n";
  throw std::runtime_error(oss.str());
}

struct message {
  clock::time_point send;
  clock::time_point recv;
};

template <typename Session>
class session : public Session, public std::enable_shared_from_this<session<Session>> {
public:
  using Session::Session;

  void start() {
    recv();
  }

private:
  void recv() {
    Session::recv(buffer_.data(), buffer_.size(), [this, self = this->shared_from_this()](const std::error_code& ec, std::size_t size) {
      if (ec) {
        if (Session::is_shutdown(ec)) {
          return;
        }
        throw std::system_error(ec, "server send");
      }
      send(buffer_.data(), size);
      recv();
    });
  }

  void send(const char* data, std::size_t size) {
    auto store = std::make_unique<char[]>(size);
    std::memcpy(store.get(), data, size);
    const auto store_data = store.get();
    Session::send(store_data, size, [self = this->shared_from_this(), store = std::move(store)](const std::error_code& ec, std::size_t) {
      if (ec) {
        if (Session::is_shutdown(ec)) {
          return;
        }
        throw std::system_error(ec, "server send");
      }
    });
  }

  std::array<char, 8 * 1024> buffer_;
};

template <typename Server>
class server {
public:
  server(std::string_view address, std::string_view service) :
    server_(io_context_, address, service) {
    // Begin accepting new connections.
    accept();

    // Start server thread.
    std::mutex mutex;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lock(mutex);
    thread_ = std::thread([this, &mutex, &cv]() {
      {
        std::lock_guard<std::mutex> lock(mutex);
        cv.notify_one();
      }
#ifndef NTSB_DEBUG
      try {
#endif
        io_context_.run();
#ifndef NTSB_DEBUG
      }
      catch (...) {
        exception_ = std::current_exception();
      }
#endif
    });

    // Wait for the server thread to fully initialize before returning from the constructor.
    cv.wait(lock);

    // Set thread affinity.
#if defined(_MSC_VER)
    if (!SetThreadAffinityMask(thread_.native_handle(), 1ull)) {
      throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "server thread affinity");
    }
#elif defined(__linux__) || defined(__FreeBSD__)
# ifdef __linux__
    cpu_set_t cpuset = {};
# else
    cpuset_t cpuset = {};
# endif
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    if (auto ev = pthread_setaffinity_np(thread_.native_handle(), sizeof(cpuset), &cpuset)) {
      throw std::system_error(std::error_code(ev, std::system_category()), "server thread affinity");
    }
#endif
  }

  void stop() {
    io_context_.stop();
  }

  void join() {
    thread_.join();
    if (exception_) {
      std::rethrow_exception(exception_);
    }
  }

  std::string endpoint() const {
    const auto ep = server_.endpoint();
    return ep.address().to_string() + ":" + std::to_string(ep.port());
  }

private:
  void accept() {
    server_.accept([this](const asio::error_code& ec, typename Server::socket socket) {
      if (ec) {
        if (Server::is_shutdown(ec)) {
          return;
        }
        throw std::system_error(ec, "server accept");
      }
      std::make_shared<session<typename Server::session>>(std::move(socket))->start();
      accept();
    });
  }

  typename Server::io_context io_context_;
  Server server_;

  std::exception_ptr exception_;
  std::thread thread_;
};

template <typename Client>
class client : public Client {
public:
  client(typename Client::io_context& io_context, std::string_view address, std::string_view service, std::size_t messages, const std::string& message) :
    Client(io_context, address, service), message_(message), message_data_(message_.data()), message_size_(message_.size()), messages_count_(messages) {
    messages_.resize(messages_count_);
  }

  void start() {
    beg_ = clock::now();
    send(0);
    recv(0, 0);
  }

  clock::time_point beg() const {
    return beg_;
  }

  clock::time_point end() const {
    return end_;
  }

  std::vector<message>& messages() {
    return messages_;
  }

  const std::vector<message>& messages() const {
    return messages_;
  }

private:
  // Sequentially sends messages.
  void send(std::size_t index) {
    if (index >= messages_count_) {
      return;
    }
    messages_[index].send = clock::now();
    Client::send(message_data_, message_size_, [this, index](const std::error_code& ec, std::size_t) {
      if (ec) {
        throw std::system_error(ec, "client send");
      }
      send(index + 1);
    });
  }

  // Sequentially reads messages.
  void recv(std::size_t index, std::size_t pos) {
    if (index >= messages_count_) {
      end_ = clock::now();
      return;
    }
    Client::recv(buffer_.data(), buffer_.size(), [this, index, pos](const std::error_code& ec, std::size_t size) {
      if (ec) {
        throw std::system_error(ec, "client recv");
      }
      auto i = index;
      auto p = pos + size;
      while (p >= message_size_ && i < messages_count_) {
        messages_[i].recv = clock::now();
        p -= message_size_;
        i += 1;
      }
      recv(i, p);
    });
  }

  clock::time_point beg_;
  clock::time_point end_;

  const std::string message_;
  const char* message_data_ = nullptr;
  std::size_t message_size_ = 0;

  std::vector<message> messages_;
  std::size_t messages_count_ = 0;

  std::array<char, 8 * 1024> buffer_;
};

template <typename Server, typename Client>
void test(std::string_view address, std::string_view service, std::size_t connections, std::size_t messages, std::size_t bytes) {
  // Chose number of threads.
  const auto threads = std::thread::hardware_concurrency();

  // Create and start server.
  server<Server> server(address, service);

#ifdef NTSB_DEBUG
  std::cout << server.endpoint() << " "
    << std::max(threads, 2u) << " threads, "
    << connections << " connections, "
    << messages << " messages, "
    << bytes << " bytes"
    << std::endl;
#endif

  // Generate message data.
  std::string message;
  message.resize(bytes);
  for (std::size_t i = 0; i < bytes; i++) {
    message[i] = '0' + (i % 10);
  }

  // Create and connect clients.
  typename Client::io_context context;
  std::vector<std::unique_ptr<client<Client>>> clients;
  for (std::size_t i = 0; i < connections; i++) {
    clients.push_back(std::make_unique<client<Client>>(context, address, service, messages, message));
  }

  std::cout << Server::type() << ' ' << Client::type() << ": " << std::flush;

  // Start client threads.
  bool started = false;
  std::mutex mutex;
  std::condition_variable cv;
  std::vector<std::thread> pool;
  pool.resize(std::max(threads, 2u) - 1);
  std::size_t thread_max = pool.size();
  std::size_t thread_cur = 0;

  for (std::size_t i = 0; i < pool.size(); i++) {
    pool[i] = std::thread([&]() {
      {
        // Wait for all client threads to fully initialize before sending messages.
        std::unique_lock<std::mutex> lock(mutex);
        if (++thread_cur < thread_max) {
          cv.wait(lock, [&]() { return thread_cur >= thread_max; });
        }
        // Start all clients.
        if (!std::exchange(started, true)) {
          for (auto& client : clients) {
            client->start();
          }
        }
      }
      cv.notify_one();
#ifndef NTSB_DEBUG
      try {
#endif
        context.run();
#ifndef NTSB_DEBUG
      }
      catch (const std::system_error& e) {
        std::lock_guard<std::mutex> lock(mutex);
        std::cout << "[" << e.code() << "] " << e.what() << std::endl;
      }
      catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(mutex);
        std::cerr << e.what() << std::endl;
      }
#endif
    });
    // Set thread affinity if there is more than one hardware thread.
    if (threads > 1) {
#if defined(_MSC_VER)
      if (!SetThreadAffinityMask(pool[i].native_handle(), 1ull << i)) {
        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()),
          "client " + std::to_string(i) + " thread affinity");
      }
#elif defined(__linux__) || defined(__FreeBSD__)
# ifdef __linux__
      cpu_set_t cpuset = {};
# else
      cpuset_t cpuset = {};
# endif
      CPU_ZERO(&cpuset);
      CPU_SET(i + 1, &cpuset);
      if (auto ev = pthread_setaffinity_np(pool[i].native_handle(), sizeof(cpuset), &cpuset)) {
        throw std::system_error(std::error_code(ev, std::system_category()), "client " + std::to_string(i) + " thread affinity");
      }
#endif
    }
  }

  // Wait for clients to finish sending/receiving messages and join client threads.
  for (auto& thread : pool) {
    thread.join();
  }

  // Stop serer and join server thread.
  server.stop();
  server.join();

  // Calculate total throughput and latency.
  auto total_bytes = connections * messages * bytes;
  auto total_mib = total_bytes / 1024.0 / 1024.0;
  clock::time_point beg = clock::time_point::max();
  clock::time_point end = clock::time_point::min();
  std::vector<std::chrono::nanoseconds> durations;
  auto min = std::chrono::nanoseconds::max();
  auto max = std::chrono::nanoseconds::min();
  for (auto& e : clients) {
    beg = std::min(beg, e->beg());
    end = std::max(end, e->end());
    for (auto& message : e->messages()) {
      const auto cur = message.recv - message.send;
      durations.emplace_back(cur);
      min = std::min(min, cur);
      max = std::max(max, cur);
    }
  }
  const auto avg = std::accumulate(durations.begin(), durations.end(), 0ns) / durations.size();
  std::nth_element(durations.begin(), durations.begin() + durations.size() / 2, durations.end());
  const auto med = durations[durations.size() / 2];

  const auto total_seconds = std::chrono::duration_cast<std::chrono::duration<double>>(end - beg).count();
  const auto total_mibps = total_mib / total_seconds;

  using milliseconds = std::chrono::duration<double, std::milli>;

  std::cout << std::fixed
    << std::setprecision(1) << total_mib << " MiB in "
    << std::setprecision(3) << total_seconds << " s, "
    << std::setprecision(1) << total_mibps << " MiB/s, "
    << "min: " << std::setprecision(3) << std::chrono::duration_cast<milliseconds>(min).count() << " ms, "
    << "max: " << std::setprecision(3) << std::chrono::duration_cast<milliseconds>(max).count() << " ms, "
    << "avg: " << std::setprecision(3) << std::chrono::duration_cast<milliseconds>(avg).count() << " ms, "
    << "med: " << std::setprecision(3) << std::chrono::duration_cast<milliseconds>(med).count() << " ms"
    << std::endl;
}

}  // namespace test

int main(int argc, char* argv[]) {
  try {
#ifdef NTSB_DEBUG
    constexpr std::size_t default_messages = 1024;
#else
    constexpr std::size_t default_messages = 4096;
#endif
    const auto server_backend = std::string_view(argc > 1 ? argv[1] : "a");
    const auto client_backend = std::string_view(argc > 2 ? argv[2] : "a");
    const auto connections = static_cast<std::size_t>(argc > 3 ? std::stoull(argv[3]) : 20);
    const auto messages = static_cast<std::size_t>(argc > 4 ? std::stoull(argv[4]) : default_messages);
    const auto bytes = static_cast<std::size_t>(argc > 5 ? std::stoull(argv[5]) : 4096);
    const auto address = std::string_view(argc > 6 ? argv[6] : "127.0.0.1");
    const auto service = std::string_view(argc > 7 ? argv[7] : "9000");

    if (server_backend == "a" && client_backend == "a") {
      test::test<asio_server, asio_client>(address, service, connections, messages, bytes);
    } else if (server_backend == "a" && client_backend == "n") {
      test::test<asio_server, net_client>(address, service, connections, messages, bytes);
    } else if (server_backend == "n" && client_backend == "a") {
      test::test<net_server, asio_client>(address, service, connections, messages, bytes);
    } else if (server_backend == "n" && client_backend == "n") {
      test::test<net_server, net_client>(address, service, connections, messages, bytes);
    } else {
      test::usage();
    }
  }
  catch (const std::system_error& e) {
    std::cout << "[" << e.code() << "] " << e.what() << std::endl;
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
