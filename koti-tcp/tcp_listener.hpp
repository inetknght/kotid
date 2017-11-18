#pragma once

#include "tcp.hpp"

#include <functional>
#include <string>
#include <utility>

namespace koti {

template <typename header_only = void>
class listener :
	public std::enable_shared_from_this<listener<>>
{
public:
	// .first is the error code
	// .second is the message, if any
	using error_descriptor = std::pair<
		boost::system::error_code,
		std::string
	>;

	enum class [[nodiscard]] error_handler_result
	{
		cancel_and_stop,
		ignore
	};

	using this_type = listener<header_only>;
	using pointer = std::shared_ptr<this_type>;
	using connection_handler_f = std::function<void(tcp::socket &&)>;
	using error_handler_f = std::function<error_handler_result()>;

	static pointer make(
		boost::asio::io_service& ios,
		tcp::endpoint endpoint,
		connection_handler_f handler,
		error_handler_f error_handler
	)
	{
		return std::make_shared<this_type>(
			ios,
			endpoint,
			handler,
			error_handler
		);
	}

	listener(
		boost::asio::io_service& ios,
		tcp::endpoint endpoint,
		connection_handler_f handler,
		error_handler_f error_handler
	)
		: ios_(ios)
		, acceptor_(ios)
		, socket_(ios)
		, handler_(handler)
		, error_handler_(error_handler)
	{
		boost::system::error_code ec;

		// Open the acceptor.
		acceptor_.open(endpoint.protocol(), ec);
		if ( ec )
		{
			ignore_failure(ec, "open");
			return;
		}

		// Bind to the server address.
		acceptor_.bind(endpoint, ec);
		if ( ec )
		{
			ignore_failure(ec, "bind");
			return;
		}

		// Start listening for connections, but do not yet start accepting.
		acceptor_.listen(boost::asio::socket_base::max_connections, ec);
		if(ec)
		{
			ignore_failure(ec, "listen");
			return;
		}

		// If a connection handler was already provided, then start accepting.
		if ( handler_ )
		{
			run();
		}
	}

	void
	run()
	{
		if( false == acceptor_.is_open() )
		{
			return;
		}
		if ( nullptr == handler_ )
		{
			return;
		}

		do_accept();
	}

	void
	do_accept()
	{
		acceptor_.async_accept(
			socket_,
			std::bind(
				&this_type::on_accept,
				shared_from_this(),
				std::placeholders::_1
			)
		);
	}

	void
	on_accept(boost::system::error_code ec)
	{
		if(ec)
		{
			if ( error_handler_result::cancel_and_stop == fail(ec, "accept") )
			{
				acceptor_.close();
				return;
			}
		}
		
		if ( nullptr != handler_ )
		{
			handler_(std::move(socket_));
		}

		socket_ = tcp::socket(ios_);
		do_accept();
	}

	const tcp::acceptor & get_acceptor() const
	{
		return acceptor_;
	}

	const error_descriptor & last_error() const
	{
		return last_ec_;
	}
	
	void clear_error() const
	{
		last_ec_ = {{}, {}};
	}

	const connection_handler_f & get_connection_handler() const
	{
		return handler_;
	}

	void set_connection_handler(connection_handler_f new_handler)
	{
		handler_ = new_handler;
	}

	const connection_handler_f & get_error_handler() const
	{
		return handler_;
	}

	void set_error_handler(error_handler_f new_handler)
	{
		error_handler_ = new_handler;
	}

protected:
	void ignore_failure(boost::system::error_code ec, std::string msg)
	{
		[[maybe_unused]] auto result = fail(ec, msg);
	}

	error_handler_result fail(boost::system::error_code ec, std::string msg)
	{
		last_ec_.first = ec;
		last_ec_.second = std::move(msg);

		if ( error_handler_ )
		{
			return error_handler_();
		}

		return error_handler_result::ignore;
	}

	asio::io_service & ios_;
	tcp::acceptor acceptor_;
	tcp::socket socket_;
	connection_handler_f handler_;
	error_handler_f error_handler_;

	error_descriptor last_ec_;
};

} // namespace koti
