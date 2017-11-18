#pragma once

#include "tcp.hpp"

#include <stdexcept>

namespace koti {

class tcp_connection
{
public:
	using pointer = std::unique_ptr<tcp_connection>;
	using socket_ptr = std::unique_ptr<tcp::socket>;

	tcp_connection(asio::io_service & ios)
		: socket_(std::make_unique<tcp::socket>(ios))
	{
	}

	tcp_connection(tcp::socket && socket)
		: socket_(std::make_unique<tcp::socket>(std::move(socket)))
	{
	}
	
	tcp_connection(std::unique_ptr<tcp::socket> && socket)
		: socket_(std::move(socket))
	{
	}

	// create a new, unconnected, tcp_connection
	static pointer make(asio::io_service & ios)
	{
		return std::make_unique<tcp_connection>(
			ios
		);
	}

	static pointer upgrade(tcp::socket && socket)
	{
		pointer connection = std::make_unique<tcp_connection>(
			std::move(socket)
		);

		return connection;
	}

	// upgrade from unique ASIO TCP socket
	static pointer upgrade(std::unique_ptr<tcp::socket> && socket)
	{
		pointer connection = std::make_unique<tcp_connection>(
			std::move(socket)
		);

		return connection;
	}

	tcp::socket & socket()
	{
		return *socket_;
	}

	const tcp::socket & socket() const
	{
		return *socket_;
	}

protected:

	socket_ptr socket_;
};

} // namespace koti
