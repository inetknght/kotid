#pragma once

extern "C" {
// #include <posix.h>
} // extern "C"

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <boost/asio.hpp>
namespace asio = boost::asio;

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <boost/beast.hpp>
namespace beast = boost::beast;

#include "httpd.hpp"
#include "options.hpp"
#include "exceptions/unhandled_value.hpp"

namespace koti {

class http_connection_list
{
public:
	auto & add_connection(
		http_connection::ptr && ptr
	)
	{
		auto at = std::find_if(
			connections_.begin(),
			connections_.end(),
			[&](const auto & ptr)
		{
			return ! ptr;
		});
		if ( connections_.end() == at )
		{
			throw std::runtime_error{"too many connections"};
		}
		(*at) = std::move(ptr);
		return *at;
	}

	void
	set_maximum_connections(
		size_t new_maximum
	)
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
};

class httpd_handler final
: public httpd_logs
, public httpd<httpd_handler>
{
public:
	using httpd::httpd;

	void
	set_connections(
		http_connection_list & connections
	)
	{
		connections_ = &connections;
	}

	void
	on_connection_closed(
		http_connection::ptr & connection
	);

	void
	on_new_connection(
		const boost::system::error_code& ec,
		local_stream::socket && socket
	);

	void
	on_new_http_connection(
		http_connection::ptr & connection
	);

protected:
	http_connection_list * connections_;
};

class application final
: public options::configurator
, public http_connection_list
, public httpd_logs
{
public:
	class exit_status {
	public:
		static exit_status success() {
			return {success_};
		}

		static exit_status failure() {
			return {failure_};
		}

		exit_status() = delete;
		exit_status(const exit_status & copy_ctor) = default;
		exit_status & operator=(const exit_status & copy_assign) = default;
		exit_status(exit_status && move_ctor) = default;
		exit_status & operator=(exit_status && move_assign) = default;

		int value() const {
			return exit_status_;
		}

		bool is_success() const {
			return success_ == exit_status_;
		}

		bool is_bad() const {
			return success_ != exit_status_;
		}

		operator bool() const {
			return is_success();
		}

	protected:
		exit_status(int status) : exit_status_(status) {}
		static constexpr const int success_ = EXIT_SUCCESS;
		static constexpr const int failure_ = EXIT_FAILURE;

		int exit_status_ = EXIT_FAILURE;
	};

	application(
		options::commandline_arguments options
	);

	options::validate
	add_options(
		options & storage
	) override;

	options::validate
	validate_configuration(
		options & storage
	) override;

	exit_status run();

	auto &
	http_server()
	{
		return http_server_;
	}

	// io_context
	asio::io_context iox_;
	std::unique_ptr<asio::io_service::work> work_;

protected:
	std::size_t maximum_connection_count_ = 1;
	options options_;
	httpd_options httpd_options_;
	std::unique_ptr<httpd_handler> http_server_;
};

} // namespace koti
