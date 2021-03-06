//
// detail/winrt_resolver_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef NET_TS_DETAIL_WINRT_RESOLVER_SERVICE_HPP
#define NET_TS_DETAIL_WINRT_RESOLVER_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <experimental/__net_ts/detail/config.hpp>

#if defined(NET_TS_WINDOWS_RUNTIME)

#include <experimental/__net_ts/ip/basic_resolver_query.hpp>
#include <experimental/__net_ts/ip/basic_resolver_results.hpp>
#include <experimental/__net_ts/detail/bind_handler.hpp>
#include <experimental/__net_ts/detail/memory.hpp>
#include <experimental/__net_ts/detail/socket_ops.hpp>
#include <experimental/__net_ts/detail/winrt_async_manager.hpp>
#include <experimental/__net_ts/detail/winrt_resolve_op.hpp>
#include <experimental/__net_ts/detail/winrt_utils.hpp>

#include <experimental/__net_ts/detail/push_options.hpp>

namespace std {
namespace experimental {
namespace net {
inline namespace v1 {
namespace detail {

template <typename Protocol>
class winrt_resolver_service :
  public service_base<winrt_resolver_service<Protocol> >
{
public:
  // The implementation type of the resolver. A cancellation token is used to
  // indicate to the asynchronous operation that the operation has been
  // cancelled.
  typedef socket_ops::shared_cancel_token_type implementation_type;

  // The endpoint type.
  typedef typename Protocol::endpoint endpoint_type;

  // The query type.
  typedef std::experimental::net::ip::basic_resolver_query<Protocol> query_type;

  // The results type.
  typedef std::experimental::net::ip::basic_resolver_results<Protocol> results_type;

  // Constructor.
  winrt_resolver_service(std::experimental::net::io_context& io_context)
    : service_base<winrt_resolver_service<Protocol> >(io_context),
      io_context_(use_service<io_context_impl>(io_context)),
      async_manager_(use_service<winrt_async_manager>(io_context))
  {
  }

  // Destructor.
  ~winrt_resolver_service()
  {
  }

  // Destroy all user-defined handler objects owned by the service.
  void shutdown()
  {
  }

  // Perform any fork-related housekeeping.
  void notify_fork(std::experimental::net::io_context::fork_event)
  {
  }

  // Construct a new resolver implementation.
  void construct(implementation_type&)
  {
  }

  // Move-construct a new resolver implementation.
  void move_construct(implementation_type&,
      implementation_type&)
  {
  }

  // Move-assign from another resolver implementation.
  void move_assign(implementation_type&,
      winrt_resolver_service&, implementation_type&)
  {
  }

  // Destroy a resolver implementation.
  void destroy(implementation_type&)
  {
  }

  // Cancel pending asynchronous operations.
  void cancel(implementation_type&)
  {
  }

  // Resolve a query to a list of entries.
  results_type resolve(implementation_type&,
      const query_type& query, std::error_code& ec)
  {
    try
    {
      using namespace Windows::Networking::Sockets;
      auto endpoint_pairs = async_manager_.sync(
          DatagramSocket::GetEndpointPairsAsync(
            winrt_utils::host_name(query.host_name()),
            winrt_utils::string(query.service_name())), ec);

      if (ec)
        return results_type();

      return results_type::create(
          endpoint_pairs, query.hints(),
          query.host_name(), query.service_name());
    }
    catch (Platform::Exception^ e)
    {
      ec = std::error_code(e->HResult,
          std::system_category());
      return results_type();
    }
  }

  // Asynchronously resolve a query to a list of entries.
  template <typename Handler>
  void async_resolve(implementation_type& impl,
      const query_type& query, Handler& handler)
  {
    bool is_continuation =
      networking_ts_handler_cont_helpers::is_continuation(handler);

    // Allocate and construct an operation to wrap the handler.
    typedef winrt_resolve_op<Protocol, Handler> op;
    typename op::ptr p = { std::experimental::net::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(query, handler);

    NET_TS_HANDLER_CREATION((io_context_.context(),
          *p.p, "resolver", &impl, 0, "async_resolve"));
    (void)impl;

    try
    {
      using namespace Windows::Networking::Sockets;
      async_manager_.async(DatagramSocket::GetEndpointPairsAsync(
            winrt_utils::host_name(query.host_name()),
            winrt_utils::string(query.service_name())), p.p);
      p.v = p.p = 0;
    }
    catch (Platform::Exception^ e)
    {
      p.p->ec_ = std::error_code(
          e->HResult, std::system_category());
      io_context_.post_immediate_completion(p.p, is_continuation);
      p.v = p.p = 0;
    }
  }

  // Resolve an endpoint to a list of entries.
  results_type resolve(implementation_type&,
      const endpoint_type&, std::error_code& ec)
  {
    ec = std::experimental::net::error::operation_not_supported;
    return results_type();
  }

  // Asynchronously resolve an endpoint to a list of entries.
  template <typename Handler>
  void async_resolve(implementation_type&,
      const endpoint_type&, Handler& handler)
  {
    std::error_code ec = std::experimental::net::error::operation_not_supported;
    const results_type results;
    io_context_.get_io_context().post(
        detail::bind_handler(handler, ec, results));
  }

private:
  io_context_impl& io_context_;
  winrt_async_manager& async_manager_;
};

} // namespace detail
} // inline namespace v1
} // namespace net
} // namespace experimental
} // namespace std

#include <experimental/__net_ts/detail/pop_options.hpp>

#endif // defined(NET_TS_WINDOWS_RUNTIME)

#endif // NET_TS_DETAIL_WINRT_RESOLVER_SERVICE_HPP
