#include "tcp_connection.hpp"

namespace koti {

tcp_connection::tcp_connection(
	asio::io_service & ios,
	const handler_functions & handlers
)
	: socket_(std::make_unique<tcp::socket>(ios))
	, handlers_(handlers)
{
}

tcp_connection::tcp_connection(
	tcp::socket && socket
	, const handler_functions & handlers
)
	: socket_(std::make_unique<tcp::socket>(std::move(socket)))
	, handlers_(handlers)
{
}

tcp_connection::tcp_connection(
	std::unique_ptr<tcp::socket> && socket
	, const handler_functions & handlers
)
	: socket_(std::move(socket))
	, handlers_(handlers)
{
}

// create a new, unconnected, tcp_connection
tcp_connection::pointer
tcp_connection::make(
	asio::io_service & ios,
	const handler_functions & handlers
)
{
	return std::make_unique<tcp_connection>(
		ios,
		handlers
	);
}

tcp_connection::pointer
tcp_connection::upgrade(
	tcp::socket && socket,
	const handler_functions & handlers
)
{
	pointer connection = std::make_unique<tcp_connection>(
		std::move(socket),
		handlers
	);

	return connection;
}

tcp_connection::pointer
tcp_connection::upgrade(
	std::unique_ptr<tcp::socket> && socket,
	const handler_functions & handlers
)
{
	pointer connection = std::make_unique<tcp_connection>(
		std::move(socket),
		handlers
	);

	return connection;
}

tcp::socket &
tcp_connection::socket()
{
	return *socket_;
}

const tcp::socket &
tcp_connection::socket() const
{
	return *socket_;
}

} // namespace koti
