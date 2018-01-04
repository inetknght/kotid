#pragma once

#include "fmt/ostream.h"
#include "spdlog/spdlog.h"
namespace spd = spdlog;

#include <cstddef>
#include <cstdint>
#include <vector>

#include "tcp_plexer.hpp"

namespace koti {

class httpd
{
public:
	using listener = koti::listener;

	httpd(
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
	on_new_http_connection(tcp_connection & pointer);

	void
	on_connection_closed(tcp_connection & pointer);

	asio::io_service & ios_;
	listener::options listener_options_;
	listener::pointer listener_;

	std::vector<tcp_connection::pointer> connections_;

	static const std::string_view httpd_logger_name_;
	static std::shared_ptr<spd::logger> logger_;
};

} // namespace koti
