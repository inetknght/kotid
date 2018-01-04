#pragma once

#include "tcp_connection.hpp"

#include <boost/beast.hpp>

namespace koti {

namespace http = boost::beast::http;

class http_session
{
public:
	using pointer = std::unique_ptr<http_session>;
	using connection_ptr = tcp_connection::pointer;

	http_session(connection_ptr && socket)
		: connection_(std::move(socket))
	{
	}

	static pointer upgrade(connection_ptr && socket)
	{
		pointer session = std::make_unique<http_session>(
			std::move(socket)
		);

		return session;
	}

	bool is_connected() const
	{
		// need to add query to tcp_connection
	}

	tcp_connection & connection()
	{
		return *connection_;
	}

	const tcp_connection & connection() const
	{
		return *connection_;
	}

protected:

	connection_ptr connection_;
	
};

} // namespace koti
