//#pragma once

//#include "fmt/ostream.h"
//#include "spdlog/spdlog.h"
//namespace spd = spdlog;

//#include <cstddef>
//#include <cstdint>
//#include <vector>

//#include "tcp_plexer.hpp"

//namespace koti {

//class httpd_logs {
//protected:
//	using logging_pointer = std::shared_ptr<spd::logger>;

//	static const std::string_view & logger_name()
//	{
//		static const std::string_view
//		logger_name{"httpd"};
//		return logger_name;
//	}

//	static logging_pointer & logger()
//	{
//		static logging_pointer
//		logger{spd::stdout_color_mt({
//				logger_name().begin(),
//				logger_name().end()
//			})};
	
//		return logger;
//	}
//};

//template <
//	class socket = tcp::socket,
//	class acceptor = tcp::acceptor
//>
//class httpd_handler
//{
//};

//template <
//	class socket = tcp::socket,
//	class acceptor = tcp::acceptor,
//	class connection_handler = connection_handler<socket>,
//	class listener_handler = listener_handler<socket, acceptor>,
//	class plexer = koti::plexer<socket, acceptor, connection_handler, >
//>
//class httpd
//	: public plexer
//{
//public:
//	using socket_type = socket;
//	using acceptor_type = acceptor;
//	using plexer_type = plexer;

//	httpd(
//		asio::io_service & ios
//	)
//		: plexer_type(ios)
//		, ios_(ios)
//	{
//	}

//	void
//	add_options(
//		koti::options & storage
//	)
//	{
//		listener_options_.add_options(storage);
//	}

//	operator bool() const
//	{
//		return false == error();
//	}

//	bool
//	error() const
//	{
//		return
//			(nullptr == listener_) ||
//			(listener_->last_error().first);
//	}

//	bool
//	is_running() const
//	{
//		return nullptr != listener_.get();
//	}

//	void
//	listen()
//	{
//		if ( (nullptr == listener_) ||
//			 (listener_->last_error().first) ||
//			 (false == listener_->listening()) )
//		{
//			listener_ = listener::make(
//				ios_,
//				tcp::endpoint{listener_options_.build()},
//				{},
//				{}
//			);
//		}
	
//		listener_->set_connection_handler(
//			std::bind(
//				&httpd::on_new_connection,
//				this,
//				std::placeholders::_1
//			)
//		);
	
//		listener_->set_error_handler(
//			std::bind(
//				&httpd::on_listener_error,
//				this
//			)
//		);
	
//		listener_->listen();
//	}

//	bool
//	listening() const
//	{
//		return (listener_) && (listener_->listening());
//	}

//	void
//	set_maximum_connections(size_t new_maximum)
//	{
//		// prefer to keep non-null (active) connections
//		std::sort(std::begin(connections_),std::end(connections_));
//		connections_.resize(new_maximum);
//	}

//	size_t
//	active_connection_count() const
//	{
//		return std::accumulate(
//			std::begin(connections_),
//			std::end(connections_),
//			0u,
//			[](size_t count, const tcp_connection::pointer & ptr)
//		{
//			return count + (bool)ptr;
//		});
//	}

//	size_t
//	maximum_connection_count() const
//	{
//		return connections_.size();
//	}

//protected:
//	listener::error_handler_result
//	on_listener_error()
//	{
//		logger_->error(
//			"listener error\t{}\t{}\t{}",
//			listener_->get_acceptor().local_endpoint(),
//			listener_->last_error().second,
//			listener_->last_error().first.message()
//		);
//		return listener::error_handler_result::cancel_and_stop;
//	}

//	void
//	on_new_connection(tcp::socket && socket)
//	{
//		tcp_connection::pointer connection = tcp_connection::upgrade(
//			std::move(socket),
//			{}
//		);
//		logger_->info("{} connected", connection->socket().remote_endpoint().address());
//	}

//	void
//	on_new_http_connection(tcp_connection & pointer);

//	void
//	on_connection_closed(tcp_connection & pointer);

//	asio::io_service & ios_;
//	listener::options listener_options_;

//	std::vector<tcp_connection::pointer> connections_;

//	static const std::string_view httpd_logger_name_;
//	static std::shared_ptr<spd::logger> logger_;
//};

//} // namespace koti
