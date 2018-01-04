#pragma once

#include "fmt/ostream.h"
#include "spdlog/spdlog.h"
namespace spd = spdlog;

#include "tcp.hpp"

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "options.hpp"

#include "tcp_connection.hpp"

namespace koti {

template <class socket = tcp::socket>
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

template <class socket = tcp::socket>
class listener_handler {
public:
	using socket_type = socket;

	enum class [[nodiscard]] error_handler_result
	{
		cancel_and_stop,
		ignore_error,
		ignore_connection
	};

	template <class tcp_listener_type>
	error_handler_result
	on_new_socket(socket_type && s)
	{
		return static_cast<tcp_listener_type*>(this)->on_new_socket(s);
	}

	template <class tcp_listener_type>
	error_handler_result
	on_listener_error(const boost::system::error_code & ec, const std::string & msg)
	{
		return static_cast<tcp_listener_type*>(this)->on_listener_error(ec, msg);
	}
};

template <class socket = tcp::socket>
std::ostream & operator<<(
	std::ostream & o,
	const typename listener_handler<socket>::error_handler_result & v
)
{
	using e = typename listener_handler<socket>::error_handler_result;
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

template <class socket>
class null_listener_handler
	: public listener_handler<socket>
{
public:
	using socket_type = typename listener_handler<socket>::socket_type;
	using error_handler_result = typename listener_handler<socket>::error_handler_result;

	void
	on_new_socket(socket_type && s)
	{
		s.close();
	}

	error_handler_result
	on_error(const boost::system::error_code &, const std::string &)
	{
		return error_handler_result::ignore;
	}
};

template <
	class socket = tcp::socket,
	class acceptor = tcp::acceptor,
	class connection_handler = null_connection_handler<socket>,
	class listener_handler = null_listener_handler<socket>,
	class connection = connection<socket, connection_handler>
>
class listener
	: virtual public koti::inheritable_shared_from_this
	, protected listener_logs<socket>
	, private listener_handler
	, private acceptor
{
public:
	using this_type = listener;

	using pointer = std::shared_ptr<this_type>;
	using socket_type = socket;
	using logs_type = listener_logs<socket_type>;
	using acceptor_type = acceptor;
	using connection_handler_type = connection_handler;
	using listener_handler_type = listener_handler;
	using connection_type = connection;

	using error_handler_result = typename listener_handler_type::error_handler_result;

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
		)
			: configurator()
			, address_(address.to_string())
			, port_(port)
		{
		}

		koti::options::validate
		validate_configuration(koti::options & storage) override
		{
			auto error = [&](const auto & e)
			{
				logs_type::logger()->error(
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
			logs_type::logger()->info("listener binding to\t{}\t{}", address_, port_);
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

	// .first is the error code
	// .second is the message, if any
	using error_descriptor = std::pair<
		boost::system::error_code,
		std::string
	>;

	static pointer make(
		boost::asio::io_service& ios,
		tcp::endpoint endpoint
	)
	{
		return std::make_shared<this_type>(
			ios,
			endpoint
		);
	}

	listener(
		asio::io_service& ios,
		tcp::endpoint endpoint
	)
		: koti::inheritable_shared_from_this() // not sure if this is correct
		, logs_type()
		, acceptor_type(ios)
		, ios_(ios)
		, socket_(ios)
	{
		boost::system::error_code ec;

		// Open the acceptor.
		acceptor_type::open(endpoint.protocol(), ec);
		if ( ec )
		{
			ignore_failure(ec, "open");
			return;
		}

		// Bind to the server address.
		acceptor_type::bind(endpoint, ec);
		if ( ec )
		{
			ignore_failure(ec, "bind");
			return;
		}

		// Start listening for connections, but do not yet start accepting.
		acceptor_type::listen(boost::asio::socket_base::max_connections, ec);
		if(ec)
		{
			ignore_failure(ec, "listen");
			return;
		}
	}

	void
	listen()
	{
		if( is_listening() )
		{
			return;
		}
	
		do_accept();
	}

	void
	stop(); // TODO: implement me

	bool
	is_listening() const
	{
		return acceptor_type::is_open();
	}

	void
	do_accept()
	{
		acceptor_type::async_accept(
			socket_,
			std::bind(
				&this_type::on_accept,
				this->shared_from_this(),
				std::placeholders::_1
			)
		);
	}

	void
	on_accept(boost::system::error_code ec)
	{
		if(ec)
		{
			auto result = fail(ec, "accept");
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

		try
		{
			error_handler_result result = 
				static_cast<listener_handler_type*>(this)->
					on_new_socket(std::move(socket_));
		}
		catch (const std::exception & e)
		{
			logs_type::logger()->critical(
				"on_accept::on_new_socket() threw exception: {}",
				e.what()
			);
			logs_type::logger()->flush();

			// CONSIDER:
			// eating the exception seems to be bad behavior
			//
			// shutdown of the acceptor will stop new problems from arising
			// but will cause denial of service
			//
			// re-throwing will force the problem to be detected upstream
			// (potentially from a crashing application)
			throw;
		}

		socket_ = socket(ios_);
		do_accept();
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

protected:
	void
	ignore_failure(boost::system::error_code ec, std::string msg)
	{
		auto result = fail(ec, msg);
		if ( error_handler_result::ignore_error != result )
		{
			logs_type::logger()->debug("ignoring failure instructions\t{}\t{}\t{}", result, ec.message(), msg);
		}
	}

	error_handler_result
	fail(boost::system::error_code ec, std::string msg)
	{
		last_ec_.first = ec;
		last_ec_.second = std::move(msg);

		error_handler_result result = error_handler_result::cancel_and_stop;

		// todo: defer logging until we have a result,
		// try/catch around error_handler_
		// if caught, log that too
		try
		{
			result = static_cast<listener_handler_type&>(*this).template on_listener_error<listener_handler_type>(ec, msg);

			logs_type::logger()->error("{}\t{}", ec.message(), msg);
		}
		catch (const std::exception & e)
		{
			logs_type::logger()->error("exception \n"
				"{}\n"
				"while processing\n"
				"{}\t{}",
				e.what(),
				ec.message(), msg);
		}

		return result;
	}

private:
	asio::io_service & ios_;

	socket_type socket_;

	error_descriptor last_ec_;
};

} // namespace koti
