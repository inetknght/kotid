#include "httpd.hpp"

#include <iostream>

namespace koti {

httpd::httpd(
	asio::io_service & ios
)
	: ios_(ios)
{
}

void
httpd::add_options(
	koti::options & storage
)
{
	listener_options_.add_options(storage);
}

httpd::operator bool() const
{
	return false == error();
}

bool
httpd::error() const
{
	return
		(nullptr == listener_) ||
		(listener_->last_error().first);
}

bool
httpd::is_running() const 
{
	return nullptr != listener_.get();
}

void
httpd::listen()
{
	if ( (nullptr == listener_) ||
		 (listener_->last_error().first) ||
		 (false == listener_->listening()) )
	{
		listener_ = listener::make(
			ios_,
			tcp::endpoint{listener_options_.build()},
			{},
			{}
		);
	}

	listener_->set_connection_handler(
		std::bind(
			&httpd::on_new_connection,
			this,
			std::placeholders::_1
		)
	);

	listener_->set_error_handler(
		std::bind(
			&httpd::on_listener_error,
			this
		)
	);

	listener_->listen();
}

listener::error_handler_result
httpd::on_listener_error()
{
	std::cerr
		<< "httpd listener error\t"
		<< listener_->last_error().second
		<< "\t" << listener_->last_error().first.message()
		<< "\n";
	return listener::error_handler_result::cancel_and_stop;
}

void
httpd::on_new_connection(tcp::socket && socket)
{
	tcp_connection::pointer connection = tcp_connection::upgrade(std::move(socket));
	std::cout << connection->socket().remote_endpoint() << "\tconnected\n";
}

} // namespace koti
