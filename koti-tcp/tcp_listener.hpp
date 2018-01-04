#pragma once

#include "fmt/ostream.h"
#include "spdlog/spdlog.h"
namespace spd = spdlog;

#include "tcp.hpp"

#include <functional>
#include <string>
#include <utility>

#include "options.hpp"

namespace koti {

class listener :
	public std::enable_shared_from_this<listener>
{
public:
	class options
		: public koti::options::configurator
	{
	public:
		using validate = koti::options::validate;
		using address_string = std::string;

		options() = default;
		options(const options & copy_ctor) = default;
		options(options && move_ctor) = default;
		options & operator=(const options & copy_assign) = default;
		options & operator=(options && move_assign) = default;
		virtual ~options() = default;

		options(
			ip::address address,
			port_number port
		);

		koti::options::validate
		validate_configuration(koti::options & storage) override;

		void
		add_options(koti::options & storage) override;

		ip::address address() const;
		ip::address address(boost::system::error_code & ec) const;
		
		const address_string & address_str() const;

		port_number & port();
		const port_number & port() const;

		tcp::endpoint build() const;

	protected:
		// options class is specifically for user input. therefore, std::string
		// because ip::address doesn't seem to play nicely with program_options
		// (as of this time)
		// this isn't as efficient as ip::address, but it's less complex than
		// supporting user input *into* native representation, and I am not yet
		// prepared to learn that route.
		address_string address_;
		port_number port_ = 0;
	};

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

	using this_type = listener;
	using pointer = std::shared_ptr<this_type>;
	using connection_handler_f = std::function<void(tcp::socket &&)>;
	using error_handler_f = std::function<error_handler_result()>;

	static pointer make(
		boost::asio::io_service& ios,
		tcp::endpoint endpoint,
		connection_handler_f handler,
		error_handler_f error_handler
	);

	listener(
		boost::asio::io_service& ios,
		tcp::endpoint endpoint,
		connection_handler_f handler,
		error_handler_f error_handler
	);

	void
	listen();

	bool
	listening() const;

	void
	do_accept();

	void
	on_accept(boost::system::error_code ec);

	const tcp::acceptor &
	get_acceptor() const;

	const error_descriptor &
	last_error() const;
	
	void
	clear_error();

	const connection_handler_f &
	get_connection_handler() const;

	void
	set_connection_handler(connection_handler_f new_handler);

	const connection_handler_f &
	get_error_handler() const;

	void
	set_error_handler(error_handler_f new_handler);

protected:
	void
	ignore_failure(boost::system::error_code ec, std::string msg);

	error_handler_result
	fail(boost::system::error_code ec, std::string msg);

	asio::io_service & ios_;
	tcp::acceptor acceptor_;
	tcp::socket socket_;
	connection_handler_f handler_;
	error_handler_f error_handler_;

	error_descriptor last_ec_;

	static const std::string_view listener_logger_name_;
	static std::shared_ptr<spd::logger> logger_;
};

} // namespace koti
