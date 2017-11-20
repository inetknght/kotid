#include "tcp_connection.hpp"

namespace koti {

tcp_connection::tcp_connection(
	asio::io_service & ios
)
	: socket_(std::make_unique<tcp::socket>(ios))
{
}

tcp_connection::tcp_connection(
	tcp::socket && socket
)
	: socket_(std::make_unique<tcp::socket>(std::move(socket)))
{
}

tcp_connection::tcp_connection(
	std::unique_ptr<tcp::socket> && socket
)
	: socket_(std::move(socket))
{
}

// create a new, unconnected, tcp_connection
tcp_connection::pointer
tcp_connection::make(
	asio::io_service & ios
)
{
	return std::make_unique<tcp_connection>(
		ios
	);
}

tcp_connection::pointer
tcp_connection::upgrade(
	tcp::socket && socket
)
{
	pointer connection = std::make_unique<tcp_connection>(
		std::move(socket)
	);

	return connection;
}

tcp_connection::pointer
tcp_connection::upgrade(
	std::unique_ptr<tcp::socket> && socket
)
{
	pointer connection = std::make_unique<tcp_connection>(
		std::move(socket)
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
