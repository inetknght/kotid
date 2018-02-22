#include "tcp_connection_test.hpp"

namespace koti {

test_clock::time_point test_clock::now_ = clock::now();

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
			EXPECT_EQ(a->on_connected(ec), connection_handler::action::normal);
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
			EXPECT_EQ(a->on_connected(ec), connection_handler::action::normal);
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
