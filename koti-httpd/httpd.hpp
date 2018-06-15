#pragma once

#include "fmt/ostream.h"
#include "spdlog/spdlog.h"
namespace spd = spdlog;

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;

#include "options.hpp"

#include "net.hpp"
#include "net_connection.hpp"
#include "net_listener.hpp"

namespace koti {

class httpd_logs {
protected:
	using logging_pointer = std::shared_ptr<spd::logger>;

	static const std::string_view & logger_name()
	{
		static const std::string_view
		logger_name{"httpd"};
		return logger_name;
	}

	static logging_pointer & logger()
	{
		static logging_pointer
		logger{spd::stdout_color_mt({
				logger_name().begin(),
				logger_name().end()
			})};
	
		return logger;
	}
};

class http_connection
: protected koti::local_stream::socket
{
public:
	using koti::local_stream::socket::basic_stream_socket;

	http_connection(koti::local_stream::socket && s)
	: koti::local_stream::socket(std::move(s))
	{
	}

	using koti::local_stream::socket::close;

	using koti::local_stream::socket::local_endpoint;
	using koti::local_stream::socket::remote_endpoint;
};

enum class http_listener_action
{
	cancel_and_stop,
	normal,
	reject_connection
};

class http_listener
: protected koti::local_stream::acceptor
{
public:
	using koti::local_stream::acceptor::acceptor;
};

class httpd_options
: public koti::options::configurator
{
public:
	options::validate
	add_options(
		options & storage
	) override
	{
		storage.descriptions().add_options()
		("--local-path,l", po::value<fs::path>(&socket_path_), "local unix socket path")
		;
		return options::validate::ok;
	}

	options::validate
	validate_configuration(
		options &
	) override
	{
		return options::validate::ok;
	}

	fs::path socket_path_;
	bool is_abstract_ = true;
};

class httpd
: protected koti::http_listener
{
public:
	using this_type = httpd;

	using protocol = koti::local_stream;
	using endpoint = typename protocol::endpoint;
	using socket = typename protocol::socket;
	using acceptor = typename protocol::acceptor;

	using http_connection_ptr = std::unique_ptr<http_connection>;

	httpd(
		asio::io_service & iox
	)
	: http_listener(iox)
	{
	}

	void
	listen(
		const local_stream::endpoint at = local_stream::local_endpoint()
	)
	{
		acceptor::open(local_stream{});
		acceptor::bind(at);
		acceptor::listen();
	}

	void
	set_maximum_connections(size_t new_maximum)
	{
		// prefer to keep non-null (active) connections
		std::sort(std::begin(connections_),std::end(connections_));
		connections_.resize(new_maximum);
	}

	size_t
	active_connection_count() const
	{
		return std::accumulate(
			std::begin(connections_),
			std::end(connections_),
			0u,
			[](size_t count, const http_connection_ptr & ptr)
		{
			return count + (bool)ptr;
		});
	}

	size_t
	maximum_connection_count() const
	{
		return connections_.size();
	}

	httpd_options &
	options()
	{
		return options_;
	}

protected:
	http_listener_action
	on_listener_error(
		const boost::system::error_code & ec,
		const std::string_view msg
	)
	{
		logger_->error(
			"listener error\t{}\t{}\t{}",
			local_endpoint(),
			ec,
			msg
		);
		return http_listener_action::cancel_and_stop;
	}

	void
	on_new_connection(local_stream::socket && socket)
	{
		http_connection_ptr connection = std::make_unique<http_connection>(
			std::move(socket)
		);
		logger_->info("{} connected", connection->remote_endpoint().path());

		on_new_http_connection(connection);
	}

	void
	on_new_http_connection(http_connection_ptr & connection)
	{
		connection->close();
		on_connection_closed(connection);
	}

	void
	on_connection_closed(http_connection_ptr & connection)
	{
		logger_->info("{} disconnected", connection->remote_endpoint().path());
	}

protected:
	httpd_options options_;
	std::vector<http_connection_ptr> connections_;

	static const std::string_view httpd_logger_name_;
	static std::shared_ptr<spd::logger> logger_;
};

} // namespace koti
