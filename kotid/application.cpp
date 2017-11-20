
#include "application.hpp"

#include <cstdint>
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace koti {

application::exit_status application::run() {

	http_server_ = std::make_unique<httpd>(ios_);

	http_server_->add_options(options_);

	auto validation = options_.configure();
	switch (validation)
	{
	case options::validate::reject:
		return exit_status::failure();
	case options::validate::reject_not_failure:
		return exit_status::success();
	case options::validate::ok:
		break;
	}

	http_server_->listen();

	work_.reset(new asio::io_service::work(ios_));
	ios_.run();
	return exit_status::success();
}

} // namespace koti
