//
// detail/win_iocp_socket_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef NET_TS_DETAIL_WIN_IOCP_SOCKET_SERVICE_HPP
#define NET_TS_DETAIL_WIN_IOCP_SOCKET_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <experimental/__net_ts/detail/config.hpp>

#if defined(NET_TS_HAS_IOCP)

#include <cstring>
#include <experimental/__net_ts/error.hpp>
#include <experimental/__net_ts/io_context.hpp>
#include <experimental/__net_ts/socket_base.hpp>
#include <experimental/__net_ts/detail/bind_handler.hpp>
#include <experimental/__net_ts/detail/buffer_sequence_adapter.hpp>
#include <experimental/__net_ts/detail/fenced_block.hpp>
#include <experimental/__net_ts/detail/handler_alloc_helpers.hpp>
#include <experimental/__net_ts/detail/handler_invoke_helpers.hpp>
#include <experimental/__net_ts/detail/memory.hpp>
#include <experimental/__net_ts/detail/mutex.hpp>
#include <experimental/__net_ts/detail/operation.hpp>
#include <experimental/__net_ts/detail/reactor_op.hpp>
#include <experimental/__net_ts/detail/select_reactor.hpp>
#include <experimental/__net_ts/detail/socket_holder.hpp>
#include <experimental/__net_ts/detail/socket_ops.hpp>
#include <experimental/__net_ts/detail/socket_types.hpp>
#include <experimental/__net_ts/detail/win_iocp_io_context.hpp>
#include <experimental/__net_ts/detail/win_iocp_null_buffers_op.hpp>
#include <experimental/__net_ts/detail/win_iocp_socket_accept_op.hpp>
#include <experimental/__net_ts/detail/win_iocp_socket_connect_op.hpp>
#include <experimental/__net_ts/detail/win_iocp_socket_recvfrom_op.hpp>
#include <experimental/__net_ts/detail/win_iocp_socket_send_op.hpp>
#include <experimental/__net_ts/detail/win_iocp_socket_service_base.hpp>

#include <experimental/__net_ts/detail/push_options.hpp>

namespace std {
namespace experimental {
namespace net {
inline namespace v1 {
namespace detail {

template <typename Protocol>
class win_iocp_socket_service :
  public service_base<win_iocp_socket_service<Protocol> >,
  public win_iocp_socket_service_base
{
public:
  // The protocol type.
  typedef Protocol protocol_type;

  // The endpoint type.
  typedef typename Protocol::endpoint endpoint_type;

  // The native type of a socket.
  class native_handle_type
  {
  public:
    native_handle_type(socket_type s)
      : socket_(s),
        have_remote_endpoint_(false)
    {
    }

    native_handle_type(socket_type s, const endpoint_type& ep)
      : socket_(s),
        have_remote_endpoint_(true),
        remote_endpoint_(ep)
    {
    }

    void operator=(socket_type s)
    {
      socket_ = s;
      have_remote_endpoint_ = false;
      remote_endpoint_ = endpoint_type();
    }

    operator socket_type() const
    {
      return socket_;
    }

    bool have_remote_endpoint() const
    {
      return have_remote_endpoint_;
    }

    endpoint_type remote_endpoint() const
    {
      return remote_endpoint_;
    }

  private:
    socket_type socket_;
    bool have_remote_endpoint_;
    endpoint_type remote_endpoint_;
  };

  // The implementation type of the socket.
  struct implementation_type :
    win_iocp_socket_service_base::base_implementation_type
  {
    // Default constructor.
    implementation_type()
      : protocol_(endpoint_type().protocol()),
        have_remote_endpoint_(false),
        remote_endpoint_()
    {
    }

    // The protocol associated with the socket.
    protocol_type protocol_;

    // Whether we have a cached remote endpoint.
    bool have_remote_endpoint_;

    // A cached remote endpoint.
    endpoint_type remote_endpoint_;
  };

  // Constructor.
  win_iocp_socket_service(std::experimental::net::io_context& io_context)
    : service_base<win_iocp_socket_service<Protocol> >(io_context),
      win_iocp_socket_service_base(io_context)
  {
  }

  // Destroy all user-defined handler objects owned by the service.
  void shutdown()
  {
    this->base_shutdown();
  }

  // Move-construct a new socket implementation.
  void move_construct(implementation_type& impl,
      implementation_type& other_impl)
  {
    this->base_move_construct(impl, other_impl);

    impl.protocol_ = other_impl.protocol_;
    other_impl.protocol_ = endpoint_type().protocol();

    impl.have_remote_endpoint_ = other_impl.have_remote_endpoint_;
    other_impl.have_remote_endpoint_ = false;

    impl.remote_endpoint_ = other_impl.remote_endpoint_;
    other_impl.remote_endpoint_ = endpoint_type();
  }

  // Move-assign from another socket implementation.
  void move_assign(implementation_type& impl,
      win_iocp_socket_service_base& other_service,
      implementation_type& other_impl)
  {
    this->base_move_assign(impl, other_service, other_impl);

    impl.protocol_ = other_impl.protocol_;
    other_impl.protocol_ = endpoint_type().protocol();

    impl.have_remote_endpoint_ = other_impl.have_remote_endpoint_;
    other_impl.have_remote_endpoint_ = false;

    impl.remote_endpoint_ = other_impl.remote_endpoint_;
    other_impl.remote_endpoint_ = endpoint_type();
  }

  // Move-construct a new socket implementation from another protocol type.
  template <typename Protocol1>
  void converting_move_construct(implementation_type& impl,
      win_iocp_socket_service<Protocol1>&,
      typename win_iocp_socket_service<
        Protocol1>::implementation_type& other_impl)
  {
    this->base_move_construct(impl, other_impl);

    impl.protocol_ = protocol_type(other_impl.protocol_);
    other_impl.protocol_ = typename Protocol1::endpoint().protocol();

    impl.have_remote_endpoint_ = other_impl.have_remote_endpoint_;
    other_impl.have_remote_endpoint_ = false;

    impl.remote_endpoint_ = other_impl.remote_endpoint_;
    other_impl.remote_endpoint_ = typename Protocol1::endpoint();
  }

  // Open a new socket implementation.
  std::error_code open(implementation_type& impl,
      const protocol_type& protocol, std::error_code& ec)
  {
    if (!do_open(impl, protocol.family(),
          protocol.type(), protocol.protocol(), ec))
    {
      impl.protocol_ = protocol;
      impl.have_remote_endpoint_ = false;
      impl.remote_endpoint_ = endpoint_type();
    }
    return ec;
  }

  // Assign a native socket to a socket implementation.
  std::error_code assign(implementation_type& impl,
      const protocol_type& protocol, const native_handle_type& native_socket,
      std::error_code& ec)
  {
    if (!do_assign(impl, protocol.type(), native_socket, ec))
    {
      impl.protocol_ = protocol;
      impl.have_remote_endpoint_ = native_socket.have_remote_endpoint();
      impl.remote_endpoint_ = native_socket.remote_endpoint();
    }
    return ec;
  }

  // Get the native socket representation.
  native_handle_type native_handle(implementation_type& impl)
  {
    if (impl.have_remote_endpoint_)
      return native_handle_type(impl.socket_, impl.remote_endpoint_);
    return native_handle_type(impl.socket_);
  }

  // Bind the socket to the specified local endpoint.
  std::error_code bind(implementation_type& impl,
      const endpoint_type& endpoint, std::error_code& ec)
  {
    socket_ops::bind(impl.socket_, endpoint.data(), endpoint.size(), ec);
    return ec;
  }

  // Set a socket option.
  template <typename Option>
  std::error_code set_option(implementation_type& impl,
      const Option& option, std::error_code& ec)
  {
    socket_ops::setsockopt(impl.socket_, impl.state_,
        option.level(impl.protocol_), option.name(impl.protocol_),
        option.data(impl.protocol_), option.size(impl.protocol_), ec);
    return ec;
  }

  // Set a socket option.
  template <typename Option>
  std::error_code get_option(const implementation_type& impl,
      Option& option, std::error_code& ec) const
  {
    std::size_t size = option.size(impl.protocol_);
    socket_ops::getsockopt(impl.socket_, impl.state_,
        option.level(impl.protocol_), option.name(impl.protocol_),
        option.data(impl.protocol_), &size, ec);
    if (!ec)
      option.resize(impl.protocol_, size);
    return ec;
  }

  // Get the local endpoint.
  endpoint_type local_endpoint(const implementation_type& impl,
      std::error_code& ec) const
  {
    endpoint_type endpoint;
    std::size_t addr_len = endpoint.capacity();
    if (socket_ops::getsockname(impl.socket_, endpoint.data(), &addr_len, ec))
      return endpoint_type();
    endpoint.resize(addr_len);
    return endpoint;
  }

  // Get the remote endpoint.
  endpoint_type remote_endpoint(const implementation_type& impl,
      std::error_code& ec) const
  {
    endpoint_type endpoint = impl.remote_endpoint_;
    std::size_t addr_len = endpoint.capacity();
    if (socket_ops::getpeername(impl.socket_, endpoint.data(),
          &addr_len, impl.have_remote_endpoint_, ec))
      return endpoint_type();
    endpoint.resize(addr_len);
    return endpoint;
  }

  // Disable sends or receives on the socket.
  std::error_code shutdown(base_implementation_type& impl,
      socket_base::shutdown_type what, std::error_code& ec)
  {
    socket_ops::shutdown(impl.socket_, what, ec);
    return ec;
  }

  // Send a datagram to the specified endpoint. Returns the number of bytes
  // sent.
  template <typename ConstBufferSequence>
  size_t send_to(implementation_type& impl, const ConstBufferSequence& buffers,
      const endpoint_type& destination, socket_base::message_flags flags,
      std::error_code& ec)
  {
    buffer_sequence_adapter<std::experimental::net::const_buffer,
        ConstBufferSequence> bufs(buffers);

    return socket_ops::sync_sendto(impl.socket_, impl.state_,
        bufs.buffers(), bufs.count(), flags,
        destination.data(), destination.size(), ec);
  }

  // Wait until data can be sent without blocking.
  size_t send_to(implementation_type& impl, const null_buffers&,
      const endpoint_type&, socket_base::message_flags,
      std::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_write(impl.socket_, impl.state_, -1, ec);

    return 0;
  }

  // Start an asynchronous send. The data being sent must be valid for the
  // lifetime of the asynchronous operation.
  template <typename ConstBufferSequence, typename Handler>
  void async_send_to(implementation_type& impl,
      const ConstBufferSequence& buffers, const endpoint_type& destination,
      socket_base::message_flags flags, Handler& handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_socket_send_op<ConstBufferSequence, Handler> op;
    typename op::ptr p = { std::experimental::net::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(impl.cancel_token_, buffers, handler);

    NET_TS_HANDLER_CREATION((io_context_, *p.p, "socket",
          &impl, impl.socket_, "async_send_to"));

    buffer_sequence_adapter<std::experimental::net::const_buffer,
        ConstBufferSequence> bufs(buffers);

    start_send_to_op(impl, bufs.buffers(), bufs.count(),
        destination.data(), static_cast<int>(destination.size()),
        flags, p.p);
    p.v = p.p = 0;
  }

  // Start an asynchronous wait until data can be sent without blocking.
  template <typename Handler>
  void async_send_to(implementation_type& impl, const null_buffers&,
      const endpoint_type&, socket_base::message_flags, Handler& handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_null_buffers_op<Handler> op;
    typename op::ptr p = { std::experimental::net::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(impl.cancel_token_, handler);

    NET_TS_HANDLER_CREATION((io_context_, *p.p, "socket",
          &impl, impl.socket_, "async_send_to(null_buffers)"));

    start_reactor_op(impl, select_reactor::write_op, p.p);
    p.v = p.p = 0;
  }

  // Receive a datagram with the endpoint of the sender. Returns the number of
  // bytes received.
  template <typename MutableBufferSequence>
  size_t receive_from(implementation_type& impl,
      const MutableBufferSequence& buffers,
      endpoint_type& sender_endpoint, socket_base::message_flags flags,
      std::error_code& ec)
  {
    buffer_sequence_adapter<std::experimental::net::mutable_buffer,
        MutableBufferSequence> bufs(buffers);

    std::size_t addr_len = sender_endpoint.capacity();
    std::size_t bytes_recvd = socket_ops::sync_recvfrom(
        impl.socket_, impl.state_, bufs.buffers(), bufs.count(),
        flags, sender_endpoint.data(), &addr_len, ec);

    if (!ec)
      sender_endpoint.resize(addr_len);

    return bytes_recvd;
  }

  // Wait until data can be received without blocking.
  size_t receive_from(implementation_type& impl,
      const null_buffers&, endpoint_type& sender_endpoint,
      socket_base::message_flags, std::error_code& ec)
  {
    // Wait for socket to become ready.
    socket_ops::poll_read(impl.socket_, impl.state_, -1, ec);

    // Reset endpoint since it can be given no sensible value at this time.
    sender_endpoint = endpoint_type();

    return 0;
  }

  // Start an asynchronous receive. The buffer for the data being received and
  // the sender_endpoint object must both be valid for the lifetime of the
  // asynchronous operation.
  template <typename MutableBufferSequence, typename Handler>
  void async_receive_from(implementation_type& impl,
      const MutableBufferSequence& buffers, endpoint_type& sender_endp,
      socket_base::message_flags flags, Handler& handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_socket_recvfrom_op<
      MutableBufferSequence, endpoint_type, Handler> op;
    typename op::ptr p = { std::experimental::net::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(sender_endp, impl.cancel_token_, buffers, handler);

    NET_TS_HANDLER_CREATION((io_context_, *p.p, "socket",
          &impl, impl.socket_, "async_receive_from"));

    buffer_sequence_adapter<std::experimental::net::mutable_buffer,
        MutableBufferSequence> bufs(buffers);

    start_receive_from_op(impl, bufs.buffers(), bufs.count(),
        sender_endp.data(), flags, &p.p->endpoint_size(), p.p);
    p.v = p.p = 0;
  }

  // Wait until data can be received without blocking.
  template <typename Handler>
  void async_receive_from(implementation_type& impl,
      const null_buffers&, endpoint_type& sender_endpoint,
      socket_base::message_flags flags, Handler& handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_null_buffers_op<Handler> op;
    typename op::ptr p = { std::experimental::net::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(impl.cancel_token_, handler);

    NET_TS_HANDLER_CREATION((io_context_, *p.p, "socket",
          &impl, impl.socket_, "async_receive_from(null_buffers)"));

    // Reset endpoint since it can be given no sensible value at this time.
    sender_endpoint = endpoint_type();

    start_null_buffers_receive_op(impl, flags, p.p);
    p.v = p.p = 0;
  }

  // Accept a new connection.
  template <typename Socket>
  std::error_code accept(implementation_type& impl, Socket& peer,
      endpoint_type* peer_endpoint, std::error_code& ec)
  {
    // We cannot accept a socket that is already open.
    if (peer.is_open())
    {
      ec = std::experimental::net::error::already_open;
      return ec;
    }

    std::size_t addr_len = peer_endpoint ? peer_endpoint->capacity() : 0;
    socket_holder new_socket(socket_ops::sync_accept(impl.socket_,
          impl.state_, peer_endpoint ? peer_endpoint->data() : 0,
          peer_endpoint ? &addr_len : 0, ec));

    // On success, assign new connection to peer socket object.
    if (new_socket.get() != invalid_socket)
    {
      if (peer_endpoint)
        peer_endpoint->resize(addr_len);
      peer.assign(impl.protocol_, new_socket.get(), ec);
      if (!ec)
        new_socket.release();
    }

    return ec;
  }

#if defined(NET_TS_HAS_MOVE)
  // Accept a new connection.
  typename Protocol::socket accept(implementation_type& impl,
      io_context* peer_io_context, endpoint_type* peer_endpoint,
      std::error_code& ec)
  {
    typename Protocol::socket peer(
        peer_io_context ? *peer_io_context : io_context_);

    std::size_t addr_len = peer_endpoint ? peer_endpoint->capacity() : 0;
    socket_holder new_socket(socket_ops::sync_accept(impl.socket_,
          impl.state_, peer_endpoint ? peer_endpoint->data() : 0,
          peer_endpoint ? &addr_len : 0, ec));

    // On success, assign new connection to peer socket object.
    if (new_socket.get() != invalid_socket)
    {
      if (peer_endpoint)
        peer_endpoint->resize(addr_len);
      peer.assign(impl.protocol_, new_socket.get(), ec);
      if (!ec)
        new_socket.release();
    }

    return peer;
  }
#endif // defined(NET_TS_HAS_MOVE)

  // Start an asynchronous accept. The peer and peer_endpoint objects
  // must be valid until the accept's handler is invoked.
  template <typename Socket, typename Handler>
  void async_accept(implementation_type& impl, Socket& peer,
      endpoint_type* peer_endpoint, Handler& handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_socket_accept_op<Socket, protocol_type, Handler> op;
    typename op::ptr p = { std::experimental::net::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    bool enable_connection_aborted =
      (impl.state_ & socket_ops::enable_connection_aborted) != 0;
    p.p = new (p.v) op(*this, impl.socket_, peer, impl.protocol_,
        peer_endpoint, enable_connection_aborted, handler);

    NET_TS_HANDLER_CREATION((io_context_, *p.p, "socket",
          &impl, impl.socket_, "async_accept"));

    start_accept_op(impl, peer.is_open(), p.p->new_socket(),
        impl.protocol_.family(), impl.protocol_.type(),
        impl.protocol_.protocol(), p.p->output_buffer(),
        p.p->address_length(), p.p);
    p.v = p.p = 0;
  }

#if defined(NET_TS_HAS_MOVE)
  // Start an asynchronous accept. The peer and peer_endpoint objects
  // must be valid until the accept's handler is invoked.
  template <typename Handler>
  void async_accept(implementation_type& impl,
      std::experimental::net::io_context* peer_io_context,
      endpoint_type* peer_endpoint, Handler& handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_socket_move_accept_op<protocol_type, Handler> op;
    typename op::ptr p = { std::experimental::net::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    bool enable_connection_aborted =
      (impl.state_ & socket_ops::enable_connection_aborted) != 0;
    p.p = new (p.v) op(*this, impl.socket_, impl.protocol_,
        peer_io_context ? *peer_io_context : io_context_,
        peer_endpoint, enable_connection_aborted, handler);

    NET_TS_HANDLER_CREATION((io_context_, *p.p, "socket",
          &impl, impl.socket_, "async_accept"));

    start_accept_op(impl, false, p.p->new_socket(),
        impl.protocol_.family(), impl.protocol_.type(),
        impl.protocol_.protocol(), p.p->output_buffer(),
        p.p->address_length(), p.p);
    p.v = p.p = 0;
  }
#endif // defined(NET_TS_HAS_MOVE)

  // Connect the socket to the specified endpoint.
  std::error_code connect(implementation_type& impl,
      const endpoint_type& peer_endpoint, std::error_code& ec)
  {
    socket_ops::sync_connect(impl.socket_,
        peer_endpoint.data(), peer_endpoint.size(), ec);
    return ec;
  }

  // Start an asynchronous connect.
  template <typename Handler>
  void async_connect(implementation_type& impl,
      const endpoint_type& peer_endpoint, Handler& handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef win_iocp_socket_connect_op<Handler> op;
    typename op::ptr p = { std::experimental::net::detail::addressof(handler),
      op::ptr::allocate(handler), 0 };
    p.p = new (p.v) op(impl.socket_, handler);

    NET_TS_HANDLER_CREATION((io_context_, *p.p, "socket",
          &impl, impl.socket_, "async_connect"));

    start_connect_op(impl, impl.protocol_.family(), impl.protocol_.type(),
        peer_endpoint.data(), static_cast<int>(peer_endpoint.size()), p.p);
    p.v = p.p = 0;
  }
};

} // namespace detail
} // inline namespace v1
} // namespace net
} // namespace experimental
} // namespace std

#include <experimental/__net_ts/detail/pop_options.hpp>

#endif // defined(NET_TS_HAS_IOCP)

#endif // NET_TS_DETAIL_WIN_IOCP_SOCKET_SERVICE_HPP