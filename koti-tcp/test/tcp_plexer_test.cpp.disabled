
#include <gtest/gtest.h>

#include "../tcp.hpp"

#include "../tcp_plexer.hpp"

namespace koti {

template <
	class socket = tcp::socket,
	class acceptor = tcp::acceptor,
	class connection_handler = null_connection_handler<socket>,
	class listener_handler = null_listener_handler<socket>,
	class plexer_handler = null_plexer_handler<socket, connection_handler, listener_handler>,
	class plexer = plexer<socket, acceptor, connection_handler, listener_handler, plexer_handler>
>
class plexer_tests
	: public testing::Test
	, public plexer_handler
{
public:
	using socket_type = socket;
	using plexer_type = plexer;

	plexer_tests() :
		testing::Test()
	{
	}

	virtual ~plexer_tests()
	{
	}

	typename plexer_type::pointer & make()
	{
		return plexer_ = plexer_type::make(
			ios_,
			{address_, port_}
		);
	}

	void
	on_new_connection(socket_type && s)
	{
		accepted_sockets_.push_back(std::move(s));
	}

	typename plexer_type::listener_type::error_handler_result
	on_listener_error()
	{
		return plexer_type::listener_type::error_handler_result::cancel_and_stop;
	}

	asio::io_service ios_;
	typename plexer_type::pointer plexer_;
	typename plexer_type::options plexer_options_;

	koti::ip::address address_;
	koti::port_number port_ = 0u;

	std::vector<socket_type> accepted_sockets_;
};

asio::io_service ios;
plexer<> test(ios);

using plexer_tests_types = ::testing::Types<>;
TYPED_TEST_CASE(plexer_tests, plexer_tests_types);

TYPED_TEST(plexer_tests, ctor_dtor)
{
	using socket_type = TypeParam;
	plexer_tests<socket_type>::make();
}

TYPED_TEST(plexer_tests, foobar)
{
	using socket_type = TypeParam;
	plexer_tests<socket_type>::make();
}

} // namespace koti
