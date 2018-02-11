#pragma once

#include "tcp.hpp"

#include <functional>
#include <stdexcept>

namespace koti {

template <class>
class connection;

class connection_handler {
public:
	enum class [[nodiscard]] action {
		disconnect,
		abort,
		normal
	};

	action
	on_connected(const boost::system::error_code &);

	action
	on_write_complete(const boost::system::error_code &, std::size_t);

	action
	on_read_complete(const boost::system::error_code & ec, std::size_t transferred);

	action
	on_error(const boost::system::error_code &);

	action
	on_disconnect();
};

class null_connection_handler
	: public connection_handler
{
public:
	using action = typename connection_handler::action;

	action
	on_connected(const boost::system::error_code &)
	{
		return action::disconnect;
	}

	action
	on_write_complete(const boost::system::error_code &, std::size_t)
	{
		return action::normal;
	}

	action
	on_read_complete(const boost::system::error_code &, std::size_t)
	{
		return action::normal;
	}

	action
	on_error(const boost::system::error_code &)
	{
		return action::abort;
	}

	action
	on_disconnect()
	{
		return action::normal;
	}
};

class buffered_read_connection_handler
{
public:
	using action = typename null_connection_handler::action;
	using buffer_type = std::vector<char>;

	buffer_type &
	read_buffer()
	{
		return read_buffer_;
	}

	const buffer_type &
	read_buffer() const
	{
		return read_buffer_;
	}

	std::size_t &
	last_read_size()
	{
		return completed_read_length_;
	}

	const std::size_t &
	last_read_size() const
	{
		return completed_read_length_;
	}

	action
	on_read_complete(const boost::system::error_code &, std::size_t transferred)
	{
		completed_read_length_ = transferred;
		return action::normal;
	}

protected:
	std::vector<char> read_buffer_;
	std::size_t completed_read_length_ = 0;
};

class buffered_write_connection_handler
{
public:
	using action = typename null_connection_handler::action;
	using buffer_type = std::vector<char>;

	buffer_type &
	write_buffer()
	{
		return write_buffer_;
	}

	const buffer_type &
	write_buffer() const
	{
		return write_buffer_;
	}

	std::size_t &
	last_write_size()
	{
		return completed_write_length_;
	}

	const std::size_t &
	last_write_size() const
	{
		return completed_write_length_;
	}

	action
	on_write_complete(const boost::system::error_code &, std::size_t transferred)
	{
		completed_write_length_ = transferred;
		return action::normal;
	}

protected:
	std::vector<char> write_buffer_;
	std::size_t completed_write_length_ = 0;
};

template <
	class connection_handler = null_connection_handler
>
class connection
	: virtual public koti::inheritable_shared_from_this
	, private tcp::socket
	, public connection_handler
{
public:
	using this_type = connection;
	using socket_type = tcp::socket;
	using connection_handler_type = connection_handler;
	using error_code = boost::system::error_code;

	using pointer = std::shared_ptr<this_type>;

	using action = typename connection_handler_type::action;

	connection(const connection & copy_ctor) = delete;
	connection(connection && move_ctor) = default;
	connection & operator=(const connection & copy_assign) = delete;
	connection & operator=(connection && move_assign) = default;

	static pointer make(
		asio::io_service & ios
	)
	{
		return protected_make_shared_enabler<this_type>(
			ios
		);
	}

	static pointer make(
		tcp::socket && s
	)
	{
		return protected_make_shared_enabler<this_type>(
			std::move(s)
		);
	}

	tcp::socket & as_socket()
	{
		return static_cast<tcp::socket&>(*this);
	}

	const tcp::socket & as_socket() const
	{
		return static_cast<const tcp::socket&>(*this);
	}

	using socket_type::get_io_service;
	using socket_type::get_option;
	using socket_type::local_endpoint;
	using socket_type::remote_endpoint;

	using socket_type::open;
	using socket_type::bind;

	//using socket_type::async_connect;
	template <class ... Args>
	auto async_connect(
		Args && ... args
	)
	{
		return socket_type::async_connect(
			std::forward<Args>(args)...,
			[&](const error_code & ec)
		{
			return static_cast<connection_handler_type&>(*this).on_connected(
				ec
			);
		});
	}

	//using socket_type::async_write_some;
	template <class ... Args>
	auto async_write_some(
		Args && ... args
	)
	{
		return socket_type::async_write_some(
			std::forward<Args>(args)...,
			[&](const error_code & ec, std::size_t transferred)
		{
			return static_cast<connection_handler_type&>(*this).on_write_complete(
				ec,
				transferred
			);
		});
	}

	//using socket_type::async_read_some;
	template <class ... Args>
	auto async_read_some(
		Args && ... args
	)
	{
		return socket_type::async_read_some(
			asio::buffer(read_buffer()),
			std::forward<Args>(args)...,
			[&](const error_code & ec, std::size_t transferred)
		{
			return static_cast<connection_handler_type&>(*this).on_read_complete(
				ec,
				transferred
			);
		});
	}

	using socket_type::shutdown;

	using socket_type::cancel;
	using socket_type::close;

	using read_buffer_type = std::vector<char>;
	read_buffer_type & read_buffer() { return read_buffer_; }
	const read_buffer_type & read_buffer() const { return read_buffer_; }

protected:
	read_buffer_type read_buffer_;

	template <class ... Args>
	connection(
		Args && ... args
	)
		: koti::inheritable_shared_from_this()
		, socket_type(std::forward<Args...>(args...))
	{
	}
};

} // namespace koti
