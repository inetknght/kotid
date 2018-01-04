#pragma once

#include "tcp.hpp"

#include <functional>
#include <stdexcept>

namespace koti {

template <class,class>
class connection;

template <class socket>
class connection_handler {
public:
	using socket_type = socket;

	enum class [[nodiscard]] action {
		disconnect,
		abort,
		normal
	};

	action
	on_connected();

	action
	on_error(const boost::system::error_code &);

	action
	on_disconnect();
};

template <class socket>
class null_connection_handler
	: public connection_handler<socket>
{
public:
	using socket_type = typename connection_handler<socket>::socket_type;
	using action = typename connection_handler<socket>::action;

	action
	on_connected(void)
	{
		return action::disconnect;
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

template <
	class socket = tcp::socket,
	class connection_handler = null_connection_handler<socket>
>
class connection
	: virtual public koti::inheritable_shared_from_this
	, private socket
{
public:
	using this_type = connection;
	using pointer = std::shared_ptr<this_type>;

	using action = typename connection_handler::action;

	connection(
		asio::io_service & ios
	)
		: socket(ios)
	{
	}

	connection(
		socket && s
	)
		: socket(std::move(s))
	{
	}

	connection(
		std::unique_ptr<socket> && s
	)
		: socket(std::move(*s))
	{
		s.release();
	}

	connection(const connection & copy_ctor) = delete;
	connection(connection && move_ctor) = default;
	connection & operator=(const connection & copy_assign) = delete;
	connection & operator=(connection && move_assign) = default;

	// create a new, unconnected, tcp_connection
	static pointer make(
		asio::io_service & ios
	)
	{
		return std::make_unique<this_type>(
			ios
		);
	}

	socket & get_socket()
	{
		return static_cast<socket &>(*this);
	}

	const socket & get_socket() const
	{
		return static_cast<const socket &>(*this);
	}

protected:
	connection_handler & get_connection_handler()
	{
		return static_cast<connection_handler &>(*this);
	}

	const connection_handler & get_connection_handler() const
	{
		return static_cast<const connection_handler &>(*this);
	}

};

} // namespace koti
