#include "tcp_plexer.hpp"

#include <iostream>
#include <numeric>

namespace koti {

const std::string_view tcp_plexer::tcp_plexer_logger_name_ = {"tcp_plexer"};
std::shared_ptr<spd::logger> tcp_plexer::logger_ = spd::stdout_color_mt({
		tcp_plexer::tcp_plexer_logger_name_.begin(),
		tcp_plexer::tcp_plexer_logger_name_.end()
	});

tcp_plexer::tcp_plexer(
	asio::io_service & ios
)
	: ios_(ios)
{
}

void
tcp_plexer::add_options(
	koti::options & storage
)
{
	listener_options_.add_options(storage);
}

tcp_plexer::operator bool() const
{
	return false == error();
}

bool
tcp_plexer::error() const
{
	return
		(nullptr == listener_) ||
		(listener_->last_error().first);
}

bool
tcp_plexer::is_running() const 
{
	return nullptr != listener_.get();
}

void
tcp_plexer::listen()
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
			&tcp_plexer::on_new_connection,
			this,
			std::placeholders::_1
		)
	);

	listener_->set_error_handler(
		std::bind(
			&tcp_plexer::on_listener_error,
			this
		)
	);

	listener_->listen();
}

bool
tcp_plexer::listening() const
{
	return (listener_) && (listener_->listening());
}

void
tcp_plexer::set_maximum_connections(size_t new_maximum)
{
	// prefer to keep non-null (active) connections
	std::sort(std::begin(connections_),std::end(connections_));
	connections_.resize(new_maximum);
}

size_t
tcp_plexer::active_connection_count() const
{
	return std::accumulate(
		std::begin(connections_),
		std::end(connections_),
		0u,
		[](size_t count, const tcp_connection::pointer & ptr)
	{
		return count + (bool)ptr;
	});
}

size_t
tcp_plexer::maximum_connection_count() const
{
	return connections_.size();
}

listener::error_handler_result
tcp_plexer::on_listener_error()
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
tcp_plexer::on_new_connection(tcp::socket && socket)
{
	tcp_connection::pointer connection = tcp_connection::upgrade(std::move(socket));
	logger_->info("{} connected", connection->socket().remote_endpoint().address());
}

void
tcp_plexer::on_connection_closed(tcp_connection::pointer & connection)
{
	logger_->info("{} disconnected", connection->socket().remote_endpoint().address());
	connection.reset();
}

} // namespace koti
