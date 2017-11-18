#pragma once

#include <cstddef>
#include <cstdint>

#include "tcp_listener.hpp"
#include "tcp_connection.hpp"

namespace koti {

template<
	typename header_only = void
>
class httpd {
public:
	using listener = koti::listener<>;

	httpd(
		asio::io_service & ios,
		ip::address listen_address,
		port_number listen_port = 80
	)
		: ios_(ios)
		, listen_address_(listen_address)
		, listen_port_(listen_port)
		, listener_(listener::make(
			ios,
			tcp::endpoint{listen_address,listen_port},
			{},
			{}
		))
	{
		// Cannot tell listener_ to run():
		// it needs a function object to call for new connections,
		// but (*this) is not ready: this ptr is not complete.
		// Therefore, we cannot yet handle callbacks.
	}

	operator bool() const
	{
		return false == error();
	}

	bool error() const
	{
		return
			(nullptr == listener_) ||
			(listener_->last_error().first);
	}

	bool is_running() const 
	{
		return listener_;
	}

	void listen()
	{
		if ( (nullptr == listener_) || (listener_->last_error().first) )
		{
			listener_ = listener::make(
				ios_,
				tcp::endpoint{listen_address_,listen_port_},
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

		listener_->run();
	}

protected:
	listener::error_handler_result on_listener_error()
	{
		std::cerr
			<< "httpd listener error\t"
			<< listener_->last_error().second
			<< "\t" << listener_->last_error().first.message()
			<< "\n";
		return listener::error_handler_result::cancel_and_stop;
	}

	void on_new_connection(tcp::socket && socket)
	{
		tcp_connection::pointer connection = tcp_connection::upgrade(std::move(socket));
		std::cout << connection->socket().remote_endpoint() << "\tconnected\n";
	}

	asio::io_service & ios_;
	ip::address listen_address_;
	port_number listen_port_;
	listener::pointer listener_;
};

} // namespace koti
