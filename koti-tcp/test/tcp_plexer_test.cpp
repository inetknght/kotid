
#include <gtest/gtest.h>

#include "../tcp.hpp"

#include "../tcp_plexer.hpp"

namespace koti {

class tcp_plexer_tests
	: public testing::Test
{
public:
	using listener = koti::listener;

	tcp_listener_tests() :
		testing::Test()
	{
	}

	virtual ~tcp_listener_tests()
	{
	}

	listener::pointer & remake()
	{
		return
			listener_ = listener::make(
				ios_,
				koti::tcp::endpoint{address, port},
				connection_handler_ = [&](koti::tcp::socket && socket)
				{
					return on_new_connection(std::move(socket));
				},
				error_handler_ = [&]()
				{
					return on_listener_error();
				}
			);
	}

	void
	on_new_connection(koti::tcp::socket && socket)
	{
		accepted_sockets_.push_back(std::move(socket));
	}

	listener::error_handler_result
	on_listener_error()
	{
		return listener::error_handler_result::cancel_and_stop;
	}

	asio::io_service ios_;
	listener::pointer listener_;

	koti::ip::address address;
	koti::port_number port = 0u;
	listener::connection_handler_f connection_handler_;
	listener::error_handler_f error_handler_;

	std::vector<koti::tcp::socket> accepted_sockets_;
};

TEST_F(tcp_listener_tests, ctor_dtor)
{
	remake();
}

} // namespace koti


TEST_F(tcp_plexer_tests, foobar)
{
}
