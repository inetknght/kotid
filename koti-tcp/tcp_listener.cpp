#include "tcp_listener.hpp"

#include <iostream>

namespace koti {

listener::options::options(
	ip::address address,
	port_number port
)
: configurator()
, address_(address.to_string())
, port_(port)
{
}

listener::options::validate
listener::options::validate_configuration(
	koti::options &
)
{
	auto error = [&](const auto & e)
	{
		std::cerr
			<< "exception caught:\n"
			<< e.what() << "\n"
			<< "when attempting to convert \"" << address_ << "\"\n"
			<< "to an IP address.\n"
			<< "Did you give a hostname instead?\n";
		return validate::reject;
	};

	try
	{
		ip::address::from_string(address_);
	}
	catch (const boost::system::system_error & e)
	{
		return error(e);
	}
	catch (const std::exception & e)
	{
		return error(e);
	}

	return validate::ok;
}

void
listener::options::add_options(
	koti::options & storage
)
{
	storage
	.add(*this)
	;

	storage
	.get_commandline()
	().add_options()
	("listen-address,a", po::value<decltype(address_)>(&address_)->required(), "address on which to bind and listen")
	("listen-port,p", po::value<decltype(port_)>(&port_)->required(), "port number on which to bind and listen")
	;
}

ip::address
listener::options::address(
) const
{
	return ip::address::from_string(address_);
}

ip::address
listener::options::address(
	boost::system::error_code & ec
) const
{
	return ip::address::from_string(address_, ec);
}

const listener::options::address_string &
listener::options::address_str() const
{
	return address_;
}

port_number & listener::options::port()
{
	return port_;
}

const port_number & listener::options::port() const
{
	return port_;
}

tcp::endpoint
listener::options::build() const
{
	std::cout << address() << ":" << port() << std::endl;
	return {address(), port()};
}

listener::pointer listener::make(
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

listener::listener(
	boost::asio::io_service& ios,
	tcp::endpoint endpoint,
	connection_handler_f handler,
	error_handler_f error_handler
)
	: enable_shared_from_this()
	, ios_(ios)
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
}

void
listener::listen()
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

bool
listener::listening() const
{
	return acceptor_.is_open();
}

void
listener::do_accept()
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
listener::on_accept(
	boost::system::error_code ec
)
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

const tcp::acceptor &
listener::get_acceptor() const
{
	return acceptor_;
}

const listener::error_descriptor &
listener::last_error() const
{
	return last_ec_;
}

void
listener::clear_error()
{
	last_ec_ = {{}, {}};
}

const listener::connection_handler_f &
listener::get_connection_handler() const
{
	return handler_;
}

void
listener::set_connection_handler(
	connection_handler_f new_handler
)
{
	handler_ = new_handler;
}

const listener::connection_handler_f &
listener::get_error_handler() const
{
	return handler_;
}

void
listener::set_error_handler(
	error_handler_f new_handler
)
{
	error_handler_ = new_handler;
}

void
listener::ignore_failure(
	boost::system::error_code ec,
	std::string msg
)
{
	[[maybe_unused]] auto result = fail(ec, msg);
}

listener::error_handler_result
listener::fail(
	boost::system::error_code ec,
	std::string msg
)
{
	last_ec_.first = ec;
	last_ec_.second = std::move(msg);

	if ( error_handler_ )
	{
		return error_handler_();
	}

	return error_handler_result::ignore;
}

} // namespace koti
