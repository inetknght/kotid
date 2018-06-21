
#include "application.hpp"

#include <cstdint>
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/beast/http/read.hpp>

namespace koti {

void
httpd_handler::on_connection_closed(
	http_connection::ptr & connection
)
{
	logger()->info("{} disconnected", connection->cached_remote_endpoint().path());
}

void
httpd_handler::on_new_connection(
	const boost::system::error_code& ec,
	koti::local_stream::socket && socket
)
{
	if ( ec )
	{
		logger()->info(
			"error\t{}",
			ec.message()
		);
		return;
	}

	if ( nullptr == connections_ )
	{
		logger()->error(
			"{} connected, but no connections list to put into",
			socket.remote_endpoint()
		);
		socket.close();
		return;
	}

	auto & connection =
	connections_->add_connection(
		std::make_unique<http_connection>(
			std::move(socket)
		));

	logger()->info(
		"{} connected",
		connection->cached_remote_endpoint().path()
	);

	on_new_http_connection(connection);
}

void
httpd_handler::on_new_http_connection(
	http_connection::ptr & connection
)
{
	connection->async_read();
}

application::application(
	options::commandline_arguments options
)
: options_(options)
{
	options_.add(*this);
}

options::validate
application::add_options(
	options & storage
)
{
	if ( ! http_server_ )
	{
		throw std::logic_error{"http server does not exist"};
	}
	storage.add(httpd_options_);
	return options::validate::ok;
}

options::validate
application::validate_configuration(
	options &
)
{
	return options::validate::ok;
}

application::exit_status application::run()
{
	work_.reset(new asio::io_service::work(iox_));

	http_server_ = std::make_unique<httpd_handler>(iox_);
	http_server_->set_connections(*this);
;
	auto configured = options_.configure();
	if ( options::validate::reject == configured )
	{
		return exit_status::failure();
	}
	if ( options::validate::reject_not_failure == configured )
	{
		return exit_status::success();
	}
	if ( options::validate::ok != configured )
	{
		throw exception::unhandled_value(configured);
	}

	http_server_->listen(httpd_options_);

	iox_.run();

	for ( auto & c : connections_ )
	{
		logger()->info(
			"{} forcing closed",
			c->cached_remote_endpoint()
		);
		c->close();
		c.reset();
	}

	return exit_status::success();
}

} // namespace koti
