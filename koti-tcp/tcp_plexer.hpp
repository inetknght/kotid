#pragma once

#include "fmt/ostream.h"
#include "spdlog/spdlog.h"
namespace spd = spdlog;

#include <cstddef>
#include <cstdint>
#include <vector>

#include "tcp_listener.hpp"
#include "tcp_connection.hpp"

namespace koti {

class tcp_plexer
{
public:
	using listener = koti::listener;

	tcp_plexer(
		asio::io_service & ios
	);

	void
	add_options(
		koti::options & storage
	);

	operator bool() const;

	bool
	error() const;

	bool
	is_running() const;

	void
	listen();

	bool
	listening() const;

	void
	set_maximum_connections(size_t new_maximum);

	size_t
	active_connection_count() const;

	size_t
	maximum_connection_count() const;

protected:
	listener::error_handler_result
	on_listener_error();

	void
	on_new_connection(tcp::socket && socket);

	void
	on_connection_closed(tcp_connection::pointer & connection);

	asio::io_service & ios_;
	listener::options listener_options_;
	listener::pointer listener_;

	std::vector<tcp_connection::pointer> connections_;

	static const std::string_view tcp_plexer_logger_name_;
	static std::shared_ptr<spd::logger> logger_;
};

} // namespace koti
