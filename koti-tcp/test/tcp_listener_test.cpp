
#include <gtest/gtest.h>

#include "../tcp.hpp"

#include "../tcp_listener.hpp"

#include "tcp_connection_test.hpp"

namespace koti {

class tcp_listener_test_handler
	: public null_listener_handler
	, public tcp_connection_test_handler
	, private listener_logs
{
public:
	using socket_type = tcp::socket;
	using acceptor_type = tcp::acceptor;
	using endpoint_type = typename acceptor_type::endpoint_type;
	using connection_handler = null_connection_handler;
	using listener_handler = null_listener_handler;
	using connection_type = koti::connection<connection_handler>;
	using TimeSource = std::chrono::steady_clock;
	using listener_options = koti::listener_options;

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
	)
	{
		listener_logs::logger()->info(
			"tcp_listener_test_handler::on_new_socket\t{}",
			remote_endpoint
		);
		accepted_sockets_.push_back(std::move(s));
	}

	error_handler_result
	on_listener_error(
		const boost::system::error_code & ec,
		const std::string & msg
	)
	{
		listener_logs::logger()->info(
			"tcp_listener_test_handler::on_listener_error\t{}\t{}",
			msg,
			ec.message()
		);
		return error_handler_result::cancel_and_stop;
	}

	std::vector<tcp::socket> accepted_sockets_;
};

class tcp_listener_tests
	: public testing::Test
	, public tcp_listener_test_handler
{
public:
	using connection_handler = tcp_listener_test_handler::connection_handler;
	using listener_handler = tcp_listener_test_handler::listener_handler;
	using connection_type = tcp_listener_test_handler::connection_type;
	using TimeSource = tcp_listener_test_handler::TimeSource;
	using listener_options = tcp_listener_test_handler::listener_options;

	using listener_type = koti::listener<
		connection_handler,
		listener_handler,
		connection_type,
		TimeSource,
		listener_options
	>;

	tcp_listener_tests(
	)
		: testing::Test()
	{
	}

	virtual ~tcp_listener_tests()
	{
	}

	typename listener_type::pointer & remake(
	)
	{
		return listener_ = listener_type::make(
			ios_
		);
	}

	typename listener_type::pointer & remake(
		const listener_type::options & listener_options
	)
	{
		return listener_ = listener_type::make(
			ios_,
			listener_options.build()
		);
	}

	asio::io_service ios_;
	typename listener_type::pointer listener_;
	typename listener_type::options listener_options_;

	std::vector<koti::tcp::socket> accepted_sockets_;
};

TEST_F(tcp_listener_tests, start_stop_listening)
{
	{
		listener_options_.address_str("127.0.0.1");
		EXPECT_NO_THROW(remake(listener_options_));
		EXPECT_TRUE(listener_->is_open());
		EXPECT_TRUE(listener_->is_listening());

		EXPECT_NO_THROW(listener_->stop());
		EXPECT_FALSE(listener_->is_open());
		EXPECT_FALSE(listener_->is_listening());

		EXPECT_NO_THROW(listener_->stop());
	}
	{
		EXPECT_NO_THROW(remake());
		EXPECT_FALSE(listener_->is_open());
		EXPECT_FALSE(listener_->is_listening());

		EXPECT_NO_THROW(listener_->stop());
		EXPECT_FALSE(listener_->is_open());
		EXPECT_FALSE(listener_->is_listening());

		EXPECT_NO_THROW(listener_->stop());
		EXPECT_NO_THROW(listener_->open());
		EXPECT_TRUE(listener_->is_open());
		EXPECT_FALSE(listener_->is_listening());
		EXPECT_EQ(listener_->local_endpoint().port(), 0);

		EXPECT_NO_THROW(listener_->listen());
		EXPECT_TRUE(listener_->is_open());
		EXPECT_TRUE(listener_->is_listening());
		EXPECT_NE(listener_->local_endpoint().port(), 0);
	}
	{
		EXPECT_NO_THROW(remake());
		EXPECT_FALSE(listener_->is_open());
		EXPECT_FALSE(listener_->is_listening());

		EXPECT_NO_THROW(listener_->listen());
		EXPECT_TRUE(listener_->is_open());
		EXPECT_TRUE(listener_->is_listening());
	}
}


} // namespace koti
