#pragma once

#include "fmt/ostream.h"
#include "spdlog/spdlog.h"
namespace spd = spdlog;

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

#include "tcp_listener.hpp"
#include "tcp_connection.hpp"

namespace koti {

template <
	class socket = tcp::socket,
	class acceptor = tcp::acceptor
>
class plexer_logs {
protected:
	using logging_pointer = std::shared_ptr<spd::logger>;

	static const std::string_view & logger_name()
	{
		static const std::string_view
		logger_name{"tcp_plexer"};
		return logger_name;
	}

	static logging_pointer & logger()
	{
		static logging_pointer
		logger{spd::stdout_color_mt({
				logger_name().begin(),
				logger_name().end()
			})};
	
		return logger;
	}
};

template <
	class socket = tcp::socket,
	class connection_handler = connection_handler<socket>,
	class listener_handler = listener_handler<socket>
>
class plexer_handler
	: public connection_handler
	, public listener_handler
{
public:
	using connection_handler_type = connection_handler;
	using listener_handler_type = listener_handler;
};

template <
	class socket = tcp::socket,
	class connection_handler = null_connection_handler<socket>,
	class listener_handler = null_listener_handler<socket>
>
class null_plexer_handler
	: public null_connection_handler<socket>
	, public null_listener_handler<socket>
{
public:
	using connection_handler_type = connection_handler;
	using listener_handler_type = listener_handler;
};

template <
	class socket = tcp::socket,
	class acceptor = tcp::acceptor,
	class connection_handler = null_connection_handler<socket>,
	class connection = connection<socket, connection_handler>,
	class listener_handler = null_listener_handler<socket>,
	class listener = listener<
		socket,
		acceptor,
		connection_handler,
		listener_handler,
		connection
	>,
	class plexer_handler = null_plexer_handler<
		socket,
		connection_handler,
		listener_handler
	>
>
class plexer
	: virtual public koti::inheritable_shared_from_this
	, protected plexer_logs<socket, acceptor>
	, public plexer_handler
	, private listener
{
public:
	using this_type = plexer;

	using socket_type = socket;
	using acceptor_type = acceptor;
	using connection_type = connection;
	using logs_type = plexer_logs<socket_type>;
	using plexer_handler_type = plexer_handler;
	using connection_handler_type = plexer_handler;
	using listener_handler_type = plexer_handler;
	using listener_type = listener;

	using pointer = std::shared_ptr<this_type>;
	using options = typename listener_type::options;

	operator const listener_type & () const
	{
		return static_cast<const listener_type&>(*this);
	}

	static typename this_type::pointer make(
			asio::io_service & ios,
			const options & options = {}
		)
	{
		return std::make_shared<this_type>(
				ios,
				options
			);
	}

	plexer(
		asio::io_service & ios,
		typename listener_type::options listener_options = {}
	)
		: koti::inheritable_shared_from_this() // not sure if this is correct
		, logs_type()
		, plexer_handler_type()
		, listener_type(ios, {listener_options.address(), listener_options.port()})
		, ios_(ios)
		, listener_options_(std::move(listener_options))
	{
	}

	void
	add_options(
		koti::options & storage
	)
	{
		listener_options_.add_options(storage);
	}

	operator bool() const
	{
		return false == error();
	}

	bool
	error() const
	{
		return listener_type::last_error().first;
	}

	bool
	is_running() const
	{
		return
			(0 < active_connection_count())
			|| listener_type::is_listening();
	}

	void
	listen()
	{
		listener_type::listen();
	}

	bool
	is_listening() const
	{
		return listener_type::is_listening();
	}

	void
	set_maximum_connections(size_t new_maximum)
	{
		// prefer to keep non-null (active) connections
		std::sort(std::begin(connections_),std::end(connections_));
		connections_.resize(new_maximum);
	}

	size_t
	active_connection_count() const
	{
		return std::accumulate(
			std::begin(connections_),
			std::end(connections_),
			0u,
			[](size_t count, const typename connection_type::pointer & ptr)
		{
			return count + (bool)ptr;
		});
	}

	size_t
	maximum_connection_count() const
	{
		return connections_.size();
	}

protected:
	typename listener_type::error_handler_result
	on_listener_error()
	{
		logs_type::logger()->error(
			"listener error\t{}\t{}\t{}",
			listener_type::get_acceptor().local_endpoint(),
			listener_type::last_error().second,
			listener_type::last_error().first.message()
		);
		return listener_type::error_handler_result::cancel_and_stop;
	}

	void
	on_new_connection(socket_type && s)
	{
		typename connection_type::pointer conn = connection_type::upgrade(std::move(s));
		logs_type::logger()->info("{} connected", conn->socket().remote_endpoint().address());
	}

	void
	on_connection_closed(typename connection_type::pointer & conn)
	{
		logs_type::logger()->info("{} disconnected", conn->socket().remote_endpoint().address());
		conn.reset();
	}

	asio::io_service & ios_;
	typename listener_type::options listener_options_;

	std::vector<typename connection_type::pointer> connections_;
};

} // namespace koti
