
#include <gtest/gtest.h>

#include "../tcp.hpp"

#include "../tcp_listener.hpp"

namespace koti {

template <class tcp_listener_tests>
class tcp_listener_test_handlers
	: public listener_handler<>
{
public:
	template <class tcp_listener_tests_type>
	void
	on_new_connection(typename tcp_listener_tests_type::listener_type::socket_type && socket)
	{
		return static_cast<tcp_listener_tests_type*>(this)->on_new_connection(socket);
	}

	template <class tcp_listener_tests_type>
	typename tcp_listener_tests_type::listener_type::error_handler_result
	on_listener_error()
	{
		return static_cast<tcp_listener_tests*>(this)->on_listener_error();
	}
};

class tcp_listener_tests
	: public testing::Test
	, public tcp_listener_test_handlers<tcp_listener_tests>
{
public:
	using listener_type = koti::listener<>;

	tcp_listener_tests(
	)
		: testing::Test()
	{
	}

	virtual ~tcp_listener_tests()
	{
	}

	typename listener_type::pointer & remake()
	{
		return
			listener_ = listener_type::make(
				ios_,
				koti::tcp::endpoint{listener_options_.address(), listener_options_.port()}
			);
	}

	void
	on_new_connection(koti::tcp::socket && socket)
	{
		accepted_sockets_.push_back(std::move(socket));
	}

	typename listener_type::error_handler_result
	on_listener_error()
	{
		return listener_type::error_handler_result::cancel_and_stop;
	}

	asio::io_service ios_;
	typename listener_type::pointer listener_;
	typename listener_type::options listener_options_;

	std::vector<koti::tcp::socket> accepted_sockets_;
};

TEST_F(tcp_listener_tests, ctor_dtor)
{
	

	remake();
}

} // namespace koti
