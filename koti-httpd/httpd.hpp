#pragma once

#include "fmt/ostream.h"
#include "spdlog/spdlog.h"
namespace spd = spdlog;

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>
#include <functional>

#include <boost/filesystem/path.hpp>
namespace fs = boost::filesystem;

#include "options.hpp"

#include "net.hpp"
#include "net_connection.hpp"
#include "net_listener.hpp"

#include <boost/beast.hpp>

namespace koti {

namespace http = boost::beast::http;

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
, protected httpd_logs
{
public:
	using koti::local_stream::socket::basic_stream_socket;
	using ptr = std::unique_ptr<http_connection>;

	const koti::local_stream::socket &
	socket() const
	{
		return *this;
	}

	koti::local_stream::socket &
	socket()
	{
		return *this;
	}

	http_connection(koti::local_stream::socket && s)
	: koti::local_stream::socket(std::move(s))
	, cached_remote_identity_{local_stream::remote_identity(*this)}
	{
	}

	using koti::local_stream::socket::close;

	using koti::local_stream::socket::local_endpoint;
	using koti::local_stream::socket::remote_endpoint;

	const koti::local_stream::ucred &
	cached_remote_identity() const
	{
		return cached_remote_identity_;
	}

	void
	on_http_header(
		const boost::system::error_code & ec,
		std::size_t bytes_transferred
	)
	{
		if ( ec )
		{
			logger()->error(
				"UID:{}\tGID:{}\tPID:{}\terror: {}",
				cached_remote_identity().uid,
				cached_remote_identity().gid,
				cached_remote_identity().pid,
				ec.message()
			);
			return;
		}
		logger()->info(
			"UID:{}\tGID:{}\tPID:{}\t{}\t{}",
			cached_remote_identity().uid,
			cached_remote_identity().gid,
			cached_remote_identity().pid,
			request_.method(),
			request_.target()
		);
	}

	void
	async_read()
	{
		http::async_read(
			socket(),
			buffer_,
			request_,
			std::bind(
				&http_connection::on_http_header,
				this,
				std::placeholders::_1,
				std::placeholders::_2
			)
		);
	}

protected:
	local_stream::ucred cached_remote_identity_;
	boost::beast::flat_buffer buffer_;
	http::request<http::string_body> request_;
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
	using koti::local_stream::acceptor::close;
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
		("local-path,l", po::value<std::string>(&socket_path_)->required(), "local unix socket path")
		("abstract", po::bool_switch(&is_abstract_), "when selected, use an abstract socket")
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

	fs::path
	path()
	const
	{
		if ( is_abstract_ )
		{
			if ( socket_path_.empty() || socket_path_.front() != '\0' )
			{
				return fs::path{std::string{1u, '\0'} + socket_path_};
			}
		}

		return socket_path_;
	}

	std::string socket_path_;
	bool is_abstract_ = true;
};

template <
	class Handler
>
class httpd
: protected koti::http_listener
{
public:
	using this_type = httpd;

	using protocol = koti::local_stream;
	using endpoint = typename protocol::endpoint;
	using socket = typename protocol::socket;
	using acceptor = typename protocol::acceptor;

	using handler = Handler;

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
		acceptor::close();
		acceptor::open(local_stream{});
		acceptor::bind(at);
		acceptor::listen();

		acceptor::async_accept(
			internal_remote_endpoint_,
			std::bind(
				&httpd<handler>::internal_on_new_connection,
				this,
				std::placeholders::_1,
				std::placeholders::_2
		));
	}

	void
	listen(
		const httpd_options &options
	)
	{
		listen({options.path().string()});
	}

	using http_listener::close;

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
			[](size_t count, const http_connection::ptr & ptr)
		{
			return count + (bool)ptr;
		});
	}

	size_t
	maximum_connection_count() const
	{
		return connections_.size();
	}

protected:
	std::vector<http_connection::ptr> connections_;

	koti::local_stream::endpoint internal_remote_endpoint_;
	void
	internal_on_new_connection(
		const boost::system::error_code& ec,
		koti::local_stream::socket && socket
	)
	{
		static_cast<handler*>(this)->on_new_connection(ec, std::move(socket));
	}

	static const std::string_view httpd_logger_name_;
	static std::shared_ptr<spd::logger> logger_;
};

} // namespace koti
