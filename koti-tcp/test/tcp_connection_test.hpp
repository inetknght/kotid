#pragma once

#include <gtest/gtest.h>

#include "../tcp_connection.hpp"

namespace koti {

class tcp_connection_test_handler;
class test_clock
{
	friend class tcp_connection_test_handler;
public:
	using clock = std::chrono::steady_clock;
	using time_point = typename clock::time_point;
	using time_duration = typename time_point::duration;

	static time_point now()
	{
		return now_;
	}

	static time_point & set_now()
	{
		return set_now(clock::now());
	}

	static time_point & set_now(time_point and_now)
	{
		return now_ = and_now;
	}

private:
	static time_point now_;
};

class tcp_connection_test_handler
	: public buffered_read_connection_handler
	, public buffered_write_connection_handler
	, public connection_timer_handler<test_clock>
{
public:
	using action = typename connection_handler::action;

	bool had_connected_ = false;
	bool had_error_ = false;
	boost::system::error_code last_error_;
	bool had_disconnected_ = false;
	bool had_write_complete_ = false;
	bool had_read_complete_ = false;

	action
	on_connected(const boost::system::error_code & ec)
	{
		had_connected_ = true;
		last_error_ = ec;
		return connection_timer_handler::on_connected(ec);
	}

	action
	on_write_complete(const boost::system::error_code & ec, std::size_t transferred)
	{
		had_write_complete_ = true;
		last_error_ = ec;
		return buffered_write_connection_handler::on_write_complete(ec, transferred);
	}

	action
	on_read_complete(const boost::system::error_code & ec, std::size_t transferred)
	{
		had_read_complete_ = true;
		last_error_ = ec;
		return buffered_read_connection_handler::on_read_complete(ec, transferred);
	}

	action
	on_disconnect()
	{
		had_disconnected_ = true;
		return action::normal;
	}
};

class tcp_connection_tests
	: public testing::Test
{
public:
	using socket_type = tcp::socket;
	using connection_type = koti::connection<tcp_connection_test_handler>;

	tcp_connection_tests(
	)
		: testing::Test()
	{
	}

	virtual ~tcp_connection_tests()
	{
	}

	typename connection_type::pointer make()
	{
		return connection_type::make(ios_);
	}

	asio::io_service ios_;

	boost::system::error_code test_ec_;
	std::vector<koti::tcp::socket> sockets_;
};

} // namespace koti
