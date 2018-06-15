
#include "application.hpp"

#include <cstdint>
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace koti {

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
	storage.add(http_server_->options());
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

	http_server_ = std::make_unique<httpd>(iox_);

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

	http_server_->listen();

	work_.reset(new asio::io_service::work(iox_));
	iox_.run();
	return exit_status::success();
}

} // namespace koti
