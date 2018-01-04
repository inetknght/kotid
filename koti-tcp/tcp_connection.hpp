#pragma once

#include "tcp.hpp"

#include <functional>
#include <stdexcept>

namespace koti {

class tcp_connection
{
public:
	using pointer = std::unique_ptr<tcp_connection>;
	using socket_ptr = std::unique_ptr<tcp::socket>;

	enum class [[nodiscard]] action {
		disconnect,
		abort,
		normal
	};

	using connected_f = std::function<action(void)>;
	using error_f = std::function<action(const boost::system::error_code &)>;
	using disconnected_f = std::function<void(tcp_connection::pointer &)>;

	struct handler_functions
	{
		connected_f connected_;
		error_f error_;
		disconnected_f disconnected_;
	};

	tcp_connection(
		asio::io_service & ios,
		const handler_functions & handlers
	);

	tcp_connection(
		tcp::socket && socket,
		const handler_functions & handlers
	);

	tcp_connection(
		std::unique_ptr<tcp::socket> && socket,
		const handler_functions & handlers
	);

	// create a new, unconnected, tcp_connection
	static pointer make(
		asio::io_service & ios,
		const handler_functions & handlers
	);

	static pointer upgrade(
		tcp::socket && socket,
		const handler_functions & handlers
	);

	// upgrade from unique ASIO TCP socket
	static pointer upgrade(
		std::unique_ptr<tcp::socket> && socket,
		const handler_functions & handlers
	);

	tcp::socket & socket();

	const tcp::socket & socket() const;

protected:

	socket_ptr socket_;
	handler_functions handlers_;
};

} // namespace koti
