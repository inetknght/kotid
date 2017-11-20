#pragma once

#include <cstddef>
#include <cstdint>

#include "tcp_listener.hpp"
#include "tcp_connection.hpp"

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

protected:
	listener::error_handler_result
	on_listener_error();

	void
	on_new_connection(tcp::socket && socket);

	asio::io_service & ios_;
	listener::options listener_options_;
	listener::pointer listener_;
};

} // namespace koti
