#include "httpd.hpp"

#include <iostream>

namespace koti {

const std::string_view httpd::httpd_logger_name_ = {"httpd"};
std::shared_ptr<spd::logger> httpd::logger_ = spd::stdout_color_mt({
		httpd::httpd_logger_name_.begin(),
		httpd::httpd_logger_name_.end()
	});

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
	logger_->error(
		"listener error\t{}\t{}\t{}",
		listener_->get_acceptor().local_endpoint(),
		listener_->last_error().second,
		listener_->last_error().first.message()
	);
	return listener::error_handler_result::cancel_and_stop;
}

void
httpd::on_new_connection(tcp::socket && socket)
{
	tcp_connection::pointer connection = tcp_connection::upgrade(std::move(socket));
	logger_->info("{} connected", connection->socket().remote_endpoint().address());
}

} // namespace koti
