
#include <gtest/gtest.h>

extern "C"
{
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
} // extern "C"

#include "httpd.hpp"
#include "cppgetenv.hpp"
#include "test_support.hpp"

#include <string>
#include <string_view>
#include <boost/filesystem.hpp>
namespace asio = boost::asio;

class httpd_test_handler
: public koti::httpd_logs
, public koti::httpd<httpd_test_handler>
{
public:
	using koti::httpd<httpd_test_handler>::httpd;

	void
	on_connection_closed(
		koti::http_connection::ptr & connection
	);

	void
	on_new_connection(
		const boost::system::error_code& ec,
		koti::local_stream::socket && socket
	);

	void
	on_new_http_connection(
		koti::http_connection::ptr & connection
	);
};

void
httpd_test_handler::on_connection_closed(
	koti::http_connection::ptr & connection
)
{
	logger()->info("{} disconnected", connection->remote_endpoint().path());
}

void
httpd_test_handler::on_new_connection(
	const boost::system::error_code& ec,
	koti::local_stream::socket && socket
)
{
	if ( ec )
	{
		logger()->info(
			"error\t{}",
			ec.message()
		);
		return;
	}

	koti::http_connection::ptr
	connection = std::make_unique<koti::http_connection>(
		std::move(socket)
	);

	logger()->info(
		"{} connected",
		connection->remote_endpoint().path()
	);

	on_new_http_connection(connection);
}

void
httpd_test_handler::on_new_http_connection(
	koti::http_connection::ptr & connection
)
{
	connection->close();
	on_connection_closed(connection);
}

class httpd_tests
: public ::testing::Test
{
public:
    using httpd = koti::httpd<httpd_test_handler>;

    boost::asio::io_context iox_;

    using connection = koti::http_connection;
    void reset()
    {
        ptr_ = std::make_unique<httpd>(iox_);
    }
    std::unique_ptr<httpd> ptr_;

    fs::path
    test_socket_path() const
    {
        auto tmp = fs::temp_directory_path();
        auto pid = ::getpid();
        tmp /= std::to_string(pid) + ".sock";
        return tmp;
    }
};

TEST_F(httpd_tests, ctor_dtor)
{
    EXPECT_NO_THROW(httpd{iox_});
    EXPECT_NO_THROW(std::make_unique<httpd>(iox_));
}

TEST_F(httpd_tests, dont_delete_me_prove_a_point_to_myself_because_i_can)
{
    auto local = koti::local_stream::endpoint{test_socket_path().string()};

    reset();
    EXPECT_NO_THROW(ptr_->listen(local));

    int native = ::socket(AF_LOCAL, SOCK_STREAM, 0);
    std::unique_ptr<int, std::function<int(int*)>> s{
        &native,
        [&](int * native_){
            return ::close(*native_);
        }};
    ASSERT_LE(0, native);

    int result;
    ASSERT_LE(0, result = ::connect(
        native,
        local.data(),
        local.size()
    ));

    
}
