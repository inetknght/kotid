
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

test_clock::time_point test_clock::now_ = clock::now();

class tcp_connection_test_handler
	: public null_connection_handler
	, public buffered_read_connection_handler
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
		return null_connection_handler::on_connected(ec);
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
		return null_connection_handler::on_disconnect();
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

TEST_F(tcp_connection_tests, ctor_dtor)
{
	auto ptr = connection_type::make(ios_);
}

TEST_F(tcp_connection_tests, connect_async_send_receive)
{
	auto
		a = connection_type::make(ios_),
		b = connection_type::make(ios_);

	tcp::endpoint local;
	local.address(ip::address::from_string("127.0.0.1"));

	a->open(asio::ip::tcp::v4());
	a->bind(local);
	b->async_connect(a->local_endpoint());

	ios_.run();
	ios_.reset();

	EXPECT_FALSE(a->had_connected_);
	EXPECT_TRUE(b->had_connected_);
	EXPECT_NE(b->last_error_, boost::system::error_code());
	EXPECT_EQ(b->last_error_, asio::error::basic_errors::connection_refused);

	b->last_error_ = {};
	b->had_connected_ = false;
	a->last_error_ = {};
	a->had_connected_ = false;

	std::shared_ptr<tcp::acceptor> acceptor = std::make_shared<tcp::acceptor>(ios_);
	EXPECT_NO_THROW(acceptor->open(ip::tcp::v4()));
	EXPECT_NO_THROW(acceptor->bind(local));
	EXPECT_NO_THROW(acceptor->listen());
	EXPECT_NO_THROW(acceptor->async_accept(
		a->as_socket(),
		[&](const boost::system::error_code & ec)->void
		{
			EXPECT_EQ(a->on_connected(ec), null_connection_handler::action::disconnect );
		}));

	EXPECT_NO_THROW(b->async_connect(acceptor->local_endpoint()));

	ios_.run();
	ios_.reset();

	// async_accept failed: a is already bound to the address.
	EXPECT_TRUE(a->had_connected_);
	EXPECT_EQ(a->last_error_, boost::asio::error::already_open);
	EXPECT_EQ(a->last_error_.message(), boost::system::error_code(boost::asio::error::already_open).message());

	// async_connect failed: remote end is not listening (aborted)
	EXPECT_TRUE(b->had_connected_);
	EXPECT_EQ(b->last_error_, boost::asio::error::connection_aborted);
	EXPECT_EQ(b->last_error_.message(), boost::system::error_code(boost::asio::error::connection_aborted).message());

	auto connectedish = test_clock::set_now(test_clock::now());
	b->last_error_ = {};
	b->had_connected_ = false;
	a->last_error_ = {};
	a->had_connected_ = false;

	// reset A, close it from the address
	EXPECT_NO_THROW(a->close());

	// reset acceptor. this may be optional.
	// EXPECT_NO_THROW(acceptor->close());
	// EXPECT_NO_THROW(acceptor->open(ip::tcp::v4()));
	// EXPECT_NO_THROW(acceptor->bind(local));
	EXPECT_NO_THROW(acceptor->listen());
	EXPECT_NO_THROW(acceptor->async_accept(
		a->as_socket(),
		[&](const boost::system::error_code & ec)->void
		{
			EXPECT_EQ(a->on_connected(ec), null_connection_handler::action::disconnect );
		}));

	EXPECT_NO_THROW(b->async_connect(acceptor->local_endpoint()));

	ios_.run();
	ios_.reset();

	auto nowish = test_clock::set_now(connectedish + std::chrono::seconds(10));
	ASSERT_LT(connectedish, nowish);
	EXPECT_EQ(connectedish, a->connection_time());
	EXPECT_EQ(connectedish, b->connection_time());
	EXPECT_EQ(a->connection_duration(), (nowish - connectedish));
	EXPECT_EQ(b->connection_duration(), (nowish - connectedish));

	EXPECT_TRUE(a->had_connected_);
	EXPECT_EQ(a->last_error_, boost::system::error_code());
	EXPECT_EQ(a->last_error_.message(), boost::system::error_code().message());

	EXPECT_TRUE(b->had_connected_);
	EXPECT_EQ(b->last_error_, boost::system::error_code());
	EXPECT_EQ(b->last_error_.message(), boost::system::error_code().message());

	EXPECT_NO_THROW(b->async_write_some(asio::buffer("foobar")));
	EXPECT_NO_THROW(a->async_read_some());

	ios_.run();
	ios_.reset();

	EXPECT_TRUE(a->had_read_complete_);
	EXPECT_EQ(a->last_error_, boost::system::error_code());
	EXPECT_EQ(a->last_error_.message(), boost::system::error_code().message());
	EXPECT_EQ(a->last_read_size(), 0u);

	EXPECT_TRUE(b->had_write_complete_);
	EXPECT_EQ(b->last_error_, boost::system::error_code());
	EXPECT_EQ(b->last_error_.message(), boost::system::error_code().message());
	EXPECT_EQ(b->last_write_size(), 1+strlen("foobar"));

	a->had_read_complete_ = false;
	a->last_error_ = boost::system::error_code();
	b->had_write_complete_ = false;
	b->last_error_ = boost::system::error_code();

	a->read_buffer().resize(1024);
	EXPECT_NO_THROW(a->async_read_some());

	ios_.run();
	ios_.reset();

	EXPECT_TRUE(a->had_read_complete_);
	EXPECT_EQ(a->last_error_, boost::system::error_code());
	EXPECT_EQ(a->last_error_.message(), boost::system::error_code().message());
	EXPECT_EQ(a->last_read_size(), 1+strlen("foobar"));
	EXPECT_EQ(a->read_buffer().size(), 1024u);

	EXPECT_NO_THROW(a->close());
	EXPECT_NO_THROW(b->close());

	ios_.run();
	ios_.reset();
}

} // namespace koti
