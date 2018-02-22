#pragma once

#include <type_traits>

#include "fmt/ostream.h"
#include "spdlog/spdlog.h"
namespace spd = spdlog;

#include "tcp.hpp"

#include "options.hpp"

#include "tcp_connection.hpp"

namespace koti {

template <class, class, class, class, class>
class listener;

class listener_logs {
protected:
	using logging_pointer = std::shared_ptr<spd::logger>;

	static const std::string_view & logger_name()
	{
		static const std::string_view
		logger_name{"listener"};
		return logger_name;
	}

	static logging_pointer & logger()
	{
		static logging_pointer
		logger{spd::stdout_color_mt({
				listener_logs::logger_name().begin(),
				listener_logs::logger_name().end()
			})};
	
		return logger;
	}
};

class listener_handler {
public:
	using socket_type = tcp::socket;
	using acceptor_type = tcp::acceptor;
	using endpoint_type = typename acceptor_type::endpoint_type;

	enum class [[nodiscard]] error_handler_result
	{
		cancel_and_stop,
		ignore_error,
		ignore_connection
	};

	void
	on_new_socket(
		socket_type && s,
		const typename socket_type::endpoint_type & remote_endpoint
	);

	error_handler_result
	on_listener_error(
		const boost::system::error_code & ec,
		const std::string & msg
	);
};

std::ostream & operator<<(
	std::ostream & o,
	const typename listener_handler::error_handler_result & v
)
{
	using e = typename listener_handler::error_handler_result;

	switch (v)
	{
	case e::cancel_and_stop:
		o << "cancel_and_stop"; break;
	case e::ignore_error:
		o << "ignore_error"; break;
	case e::ignore_connection:
		o << "ignore_connection"; break;
	}

	return o;
}

class null_listener_handler
	: public listener_handler
{
public:
	using socket_type = tcp::socket;
	using acceptor_type = tcp::acceptor;
	using error_handler_result = typename listener_handler::error_handler_result;

	void
	on_new_socket(
		socket_type && s,
		const typename socket_type::endpoint_type &
	)
	{
		s.close();
	}

	error_handler_result
	on_listener_error(
		const boost::system::error_code &,
		const std::string &
	)
	{
		return error_handler_result::ignore_error;
	}
};

class listener_options
	: public koti::options::configurator
	, virtual private koti::listener_logs
{
public:
	using validate = koti::options::validate;
	using address_string = std::string;

	listener_options() = default;
	listener_options(const listener_options & copy_ctor) = default;
	listener_options(listener_options && move_ctor) = default;
	listener_options & operator=(const listener_options & copy_assign) = default;
	listener_options & operator=(listener_options && move_assign) = default;
	virtual ~listener_options() = default;

	listener_options(
		ip::address listen_address,
		port_number listen_port = 0
	)
		: configurator()
		, address_(listen_address.to_string())
		, port_(listen_port)
	{
	}

	koti::options::validate
	validate_configuration(koti::options &) override
	{
		auto error = [&](const auto & e)
		{
			listener_logs::logger()->error(
				"exception caught\t"
				"when attempting to convert \"{}\" to an IP address\t"
				"Did you give a hostname instead?\t"
				"{}"
				, address_
				, e.what()
			);
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
	add_options(koti::options & storage) override
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

	ip::address address() const
	{
		return ip::address::from_string(address_);
	}

	ip::address address(boost::system::error_code & ec) const
	{
		return ip::address::from_string(address_, ec);
	}

	const address_string & address_str() const
	{
		return address_;
	}

	void address_str(address_string && new_address)
	{
		address_ = std::move(new_address);
	}

	port_number & port()
	{
		return port_;
	}

	const port_number & port() const
	{
		return port_;
	}

	tcp::endpoint build() const
	{
		return {address(), port()};
	}

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

template <
	class connection_handler = null_connection_handler,
	class listener_handler = null_listener_handler,
	class connection = connection<connection_handler>,
	class TimeSource = std::chrono::steady_clock,
	class listener_options = listener_options
>
class listener
	: virtual public koti::inheritable_shared_from_this
	, virtual public listener_logs
	, public listener_handler
	, public listener_options
	, private kotipp::timestamp<TimeSource>
	, private tcp::acceptor
{
public:
	using this_type = listener;

	using pointer = std::shared_ptr<this_type>;
	using socket_type = tcp::socket;
	using acceptor_type = tcp::acceptor;
	using endpoint_type = typename acceptor_type::endpoint_type;
	using logs_type = listener_logs;
	using connection_handler_type = connection_handler;
	using listener_handler_type = listener_handler;
	using connection_type = connection;

	using error_handler_result = typename listener_handler_type::error_handler_result;

	using time_source = TimeSource;
	using time_point = typename time_source::time_point;
	using time_duration = typename time_point::duration;

	using options = listener_options;

	// .first is the error code
	// .second is the message, if any
	using error_descriptor = std::pair<
		boost::system::error_code,
		std::string
	>;

	static pointer make(
		boost::asio::io_service & ios
	)
	{
		return koti::protected_make_shared_enabler<this_type>(ios);
	}

	static pointer make(
		boost::asio::io_service & ios,
		tcp::endpoint endpoint
	)
	{
		return koti::protected_make_shared_enabler<this_type>(ios, endpoint);
	}

	void set_reuse_address(bool to_reuse = true)
	{
		typename acceptor_type::reuse_address option(to_reuse);
		tcp::acceptor::set_option(option);
	}

	bool is_reuse_address() const
	{
		typename acceptor_type::reuse_address option;
		tcp::acceptor::get_option(option);
		return option.value();
	}

	int
	set_linger(
		bool is_set = true,
		int timeout_in_seconds = 5
	)
	{
		typename acceptor_type::linger option(is_set, timeout_in_seconds);
		tcp::acceptor::set_option(option);
		return option.timeout();
	}

	bool
	is_linger() const
	{
		typename acceptor_type::linger option;
		tcp::acceptor::get_option(option);
		return option.timeout();
	}

	bool
	open(
	)
	{
		if ( is_open() )
		{
			return true;
		}

		last_ec_ = {};
		acceptor_type::open(boost::asio::ip::tcp::v4(), last_ec_.first);
		listener_logs::logger()->debug(
			"open():{}",
			last_ec_.first
		);
		if ( last_ec_.first )
		{
			last_ec_.second = "open()";
			log_error();
			return false;
		}

		return true;
	}

	bool
	bind(
		tcp::endpoint endpoint = tcp::endpoint(
			ip::address::from_string("127.0.0.1"), 0
		)
	)
	{
		if ( is_bound() )
		{
			return true;
		}

		last_ec_ = {};
		if ( false == open() )
		{
			listener_logs::logger()->debug(
				"bind({})::open()",
				endpoint
			);
			return false;
		}

		acceptor_type::bind(
			endpoint,
			last_ec_.first
		);
		listener_logs::logger()->debug(
			"bind({}):{}",
			endpoint,
			last_ec_.first
		);
		if ( last_ec_.first )
		{
			last_ec_.second = fmt::format(
				"bind({})::bind()",
				endpoint
			);
			log_error();
			return false;
		}

		local_endpoint_ = local_endpoint();
		if ( last_ec_.first )
		{
			last_ec_.second = fmt::format(
				"bind({})::local_endpoint()",
				local_endpoint_
			);
			log_error();
			close();
			return false;
		}

		listener_logs::logger()->info(
			"listener bound to\t{}",
			local_endpoint_
		);
		return true;
	}

	template <
		class ... Args
	>
	bool
	listen()
	{
		if ( is_listening() )
		{
			return true;
		}

		last_ec_ = {};
		if ( false == bind() )
		{
			listener_logs::logger()->debug(
				"listen()::bind()"
			);
			return false;
		}

		acceptor_type::listen(
			acceptor_type::max_listen_connections,
			last_ec_.first
		);
		if ( last_ec_.first )
		{
			last_ec_.second = "listen()::listen()";
			log_error();
			return false;
		}

		do_accept();
		return true;
	}

	void
	stop()
	{
		acceptor_type::close();
	}

	bool
	is_open() const
	{
		return acceptor_type::is_open();
	}

	bool
	is_bound()
	{
		if ( false == is_open() )
		{
			return false;
		}

		last_ec_ = {};
		local_endpoint_  = acceptor_type::local_endpoint(last_ec_.first);
		if ( last_ec_.first )
		{
			last_ec_.second = "is_bound()::local_endpoint()";
			log_error();
			return false;
		}

		return true;
	}

	bool
	is_listening()
	{
		if ( false == is_bound() )
		{
			return false;
		}

		last_ec_ = {};
		is_listening_option option;
		acceptor_type::get_option(option, last_ec_.first);
		if ( last_ec_.first )
		{
			last_ec_.second = "is_listening()";
			log_error();
			return false;
		}

		return option;
	}

	endpoint_type
	local_endpoint()
	{
		if ( false == is_bound() )
		{
			return {};
		}

		return local_endpoint_;
	}

	void
	do_accept()
	{
		acceptor_type::async_accept(
			socket_,
			[=](const boost::system::error_code & ec)
		{
			return this->on_accept(ec);
		});
	}

	void
	on_accept(const boost::system::error_code & ec)
	try
	{
		if(ec)
		{
			auto result = fail(ec, "accept()");
			if ( error_handler_result::cancel_and_stop == result )
			{
				acceptor_type::close();
				return;
			}
			if ( error_handler_result::ignore_connection == result )
			{
				return;
			}
			assert(error_handler_result::ignore_error == result);
		}

		tcp::socket::endpoint_type remote = socket_.remote_endpoint(
			last_ec_.first
		);
		if ( last_ec_.first )
		{
			last_ec_.second = fmt::format(
				"on_accept({})::remote_endpoint()",
				remote
			);
			log_error();
		}

		try
		{
			listener_handler::on_new_socket(std::move(socket_), remote);
		}
		catch (const std::exception & e)
		{
			logs_type::logger()->critical(
				"on_accept::on_new_socket({}) threw exception: {}",
				remote,
				e.what()
			);

			// Exceptions are for _exceptional circumstances_, so I don't really
			// care about the performance penalty of a flush after logging the
			// circumstances involved.
			logs_type::logger()->flush();

			// CONSIDER:
			// eating the exception seems to be bad behavior
			//
			// shutdown of the acceptor will stop new problems from arising
			// but will cause denial of service
			//
			// re-throwing will force the problem to be detected upstream
			// (potentially from a crashing application)
			// ... and may also cause denial of service ...
			// so if we can't avoid the possibility, then let's at least
			// try to get a crashdump with the unhandled exception
			socket_.close();
			throw;
		}

		// Specifically: update timestamp upon _exit_ of on_accept()
		kotipp::timestamp<TimeSource>::stamp_now();
		socket_ = tcp::socket(ios_);
		do_accept();
	}
	catch (...)
	{
		// Specifically: update timestamp upon _exit_ of on_accept()
		kotipp::timestamp<TimeSource>::stamp_now();
		throw;
	}

	const error_descriptor &
	last_error() const
	{
		return last_ec_;
	}

	void
	clear_error()
	{
		last_ec_ = {{}, {}};
	}

	time_point
	last_accept_time() const
	{
		return kotipp::timestamp<TimeSource>::previous();
	}

	time_duration
	connection_time() const
	{
		return kotipp::timestamp<TimeSource>::duration_since_stamp();
	}

protected:

#define BOOST_ASIO_OS_DEF_SO_ACCEPTCONN SO_ACCEPTCONN
	using is_listening_option = asio::detail::socket_option::boolean<
		BOOST_ASIO_OS_DEF(SOL_SOCKET),
		BOOST_ASIO_OS_DEF(SO_ACCEPTCONN)
	>;

	listener(
		asio::io_service & ios
	)
		: koti::inheritable_shared_from_this()
		, logs_type()
		, listener_handler()
		, listener_options()
		, kotipp::timestamp<TimeSource>()
		, acceptor_type(ios)
		, ios_(ios)
		, socket_(ios)
	{
		
	}

	listener(
		asio::io_service & ios,
		tcp::endpoint desired_local_endpoint
	)
		: koti::inheritable_shared_from_this()
		, logs_type()
		, listener_handler()
		, listener_options()
		, kotipp::timestamp<TimeSource>()
		, acceptor_type(ios)
		, ios_(ios)
		, socket_(ios)
	{
		// Open the acceptor.
		if ( false == open() )
		{
			ignore_failure(
				last_ec_.first,
				fmt::format("{0}{1}{3}",
					"listener(ios_,",
					desired_local_endpoint,
					")::open()"
				)
			);
			return;
		}

		// Bind to the server address.
		bind(desired_local_endpoint);
		if ( last_ec_.first )
		{
			ignore_failure(last_ec_.first, "bind");
			return;
		}

		// Start listening for connections, but do not yet start accepting.
		acceptor_type::listen(boost::asio::socket_base::max_connections, last_ec_.first );
		if( last_ec_.first )
		{
			ignore_failure(last_ec_.first, "listen");
			return;
		}
	}

	void
	ignore_failure(boost::system::error_code ec, std::string msg)
	{
		auto result = fail(ec, msg);
		if ( error_handler_result::ignore_error != result )
		{
			logs_type::logger()->debug("ignoring failure instructions\t{}\t{}\t{}", result, ec.message(), msg);
		}
	}

	void
	ignore_failure(std::string msg)
	{
		auto result = fail(msg);
		return ignore_failure(result);
	}

	void
	ignore_failure(error_handler_result result)
	{
		if ( error_handler_result::ignore_error != result )
		{
			logs_type::logger()->debug(
				"ignoring failure instructions\t{}\t{}\t{}",
				result,
				last_ec_.first.message(),
				last_ec_.second
			);
		}
	}

	error_handler_result
	fail(boost::system::error_code ec, std::string msg)
	{
		last_ec_.first = std::move(ec);
		return fail(std::move(msg));
	}

	error_handler_result
	fail(std::string msg)
	{
		last_ec_.second = std::move(msg);
		return fail();
	}

	error_handler_result
	fail()
	{
		error_handler_result result = error_handler_result::cancel_and_stop;

		// todo: defer logging until we have a result,
		// try/catch around error_handler_
		// if caught, log that too
		try
		{
			result = listener_handler::on_listener_error(
				last_ec_.first,
				last_ec_.second
			);
			log_error();
		}
		catch (const std::exception & e)
		{
			logs_type::logger()->error("exception \n"
				"{}\n"
				"while processing\n"
				"{}\t{}",
				e.what(),
				last_ec_.first.message(), last_ec_.second
			);
		}

		return result;
	}

private:
	void
	log_error() const
	{
		listener_logs::logger()->error(
			"{}:\t{}",
			last_ec_.second,
			last_ec_.first.message()
		);
	}

	asio::io_service & ios_;
	tcp::socket socket_;
	error_descriptor last_ec_;
	endpoint_type local_endpoint_;
};

} // namespace koti
