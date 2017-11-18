
#include "application.hpp"

#include <cstdint>
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace koti {

application::options::options(int argc, char **argv) {
	assign(argc, argv);
}

void application::options::assign(int argc, char **argv) {

}

application::exit_status application::run() {
	http_server_ = std::make_unique<httpd<>>(
		ios_,
		ip::address{},
		8080
	);
	http_server_->listen();

	work_.reset(new asio::io_service::work(ios_));
	ios_.run();
	return exit_status::success();
}

} // namespace koti
