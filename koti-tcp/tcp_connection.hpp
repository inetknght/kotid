#pragma once

#include "tcp.hpp"

#include <stdexcept>

namespace koti {

class tcp_connection
{
public:
	using pointer = std::unique_ptr<tcp_connection>;
	using socket_ptr = std::unique_ptr<tcp::socket>;

	tcp_connection(asio::io_service & ios);

	tcp_connection(tcp::socket && socket);
	
	tcp_connection(std::unique_ptr<tcp::socket> && socket);

	// create a new, unconnected, tcp_connection
	static pointer make(asio::io_service & ios);

	static pointer upgrade(tcp::socket && socket);

	// upgrade from unique ASIO TCP socket
	static pointer upgrade(std::unique_ptr<tcp::socket> && socket);

	tcp::socket & socket();

	const tcp::socket & socket() const;

protected:

	socket_ptr socket_;
};

} // namespace koti
