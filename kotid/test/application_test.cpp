extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
} // extern "C"

#include "../application.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <functional>

#include "exceptions/timeout.hpp"

#include "test_support.hpp"

// This should be defined from the CMakeLists.txt, ADD_DEFINITIONS(-DTEST_DIR_ROOT=...)
#ifndef TEST_DIR_ROOT
#define TEST_DIR_ROOT ""
#endif

class application_test : public ::testing::Test {
public:

	void
	reset()
	{
		app_.reset();
		configure();
		configure_raw();
		int argc = args_.size();
		char ** argv = const_cast<char**>(args_raw_.data());
		app_ = std::make_unique<koti::application>(
			koti::options::commandline_arguments{argc, argv}
		);
	}

	koti::application::exit_status
	run()
	{
		if ( ! app_ )
		{
			reset();
		}
		return app_->run();
	}

	void
	configure()
	{
		if ( args_.empty() )
		{
			args_.push_back("./test");
		}
	}

	void configure_raw()
	{
		args_raw_.clear();
		args_raw_.reserve(args_.size() + 1);
		for ( const auto & arg : args_ )
		{
			args_raw_.push_back(arg.c_str());
		}
		args_raw_.push_back(nullptr);
	}

	std::shared_ptr<koti::application> app_;
	std::vector<std::string> args_;
	std::vector<const char*> args_raw_;
};

TEST_F(application_test, no_parameters) {
    reset();
    EXPECT_NO_THROW(run());
}

TEST_F(application_test, local_path) {
    int native = ::socket(AF_LOCAL, SOCK_STREAM, 0);
    ASSERT_LE(0, native);

    int result = ::fcntl(native, F_SETFD, O_NONBLOCK);
    ASSERT_LE(0, result);
    result = -1;

    fs::path test_socket_path = koti::test::test_socket_path();

    auto until = std::chrono::system_clock::now() + std::chrono::seconds(1);

    std::vector<char>
        native_readbuf, native_writebuf;
    native_readbuf.resize(4096);
    native_writebuf.reserve(4096);

    int total_write = 0;

    std::function<void()> do_read = [&]()
    {
        app_->iox_.stop();
    };

    std::function<void()> do_write = [&]()
    {
        native_writebuf =
        {
            'G','E','T',' ','/',' ','H','T','T','P','/','1','.','1','\r','\n',
            'H','o','s','t',':',' ','t','e','s','t','\r','\n',
            '\r','\n'
        };
        if ( (static_cast<int>(native_writebuf.size()) - total_write) < 0 )
        {
            throw std::runtime_error{"youdidbad"};
        }
        if ( native_writebuf.size() == static_cast<std::size_t>(total_write) )
        {
            asio::post(app_->iox_, do_read);
            return;
        }
        if ( 0 < total_write )
        {
            native_writebuf.erase(native_writebuf.begin(), native_writebuf.begin() + total_write);
        }
        result = ::write(native, native_writebuf.data(), native_writebuf.size());
        if ( result < 0 )
        {
            int e = errno;
            switch (e)
            {
            case EAGAIN: [[fallthrough]];
            case EINTR:
                if ( until <= std::chrono::system_clock::now() )
                {
                    throw koti::exception::timeout{std::string{"timed out trying to write to "} + test_socket_path.string()};
                }
                break;
            default:
                throw std::runtime_error{std::string{"unhandled result from ::write(): "} + strerror(e)};
            }
        }
        else
        {
            total_write += result;
        }
        asio::post(app_->iox_, do_write);
    };

    std::function<void()> do_connect = [&]()
    {
        union {
            sockaddr local_addr_base;
            sockaddr_un local_addr;
        };
        memset(&local_addr, '\0', sizeof(local_addr));
        if ( false == test_socket_path.empty() )
        {
            if ( sizeof(local_addr.sun_path) - 1 < test_socket_path.size() )
            {
                throw std::logic_error{"test socket path is too long"};
            }
            local_addr.sun_family = AF_UNIX;
            memcpy(local_addr.sun_path, test_socket_path.c_str(), test_socket_path.size());
            local_addr.sun_path[test_socket_path.size()] = '\0';
        }

        result = ::connect(native, &local_addr_base, sizeof(local_addr));
        if ( result < 0 )
        {
            int e = errno;
            switch (e)
            {
            case EINPROGRESS: break;
            case EAGAIN: break;
            case ENOENT: break;
            default:
                throw std::runtime_error{std::string{"unhandled error from ::connect(): "} + strerror(e)};
            }
            if ( until <= std::chrono::system_clock::now() )
            {
                throw koti::exception::timeout{std::string{"timed out trying to connect to "} + test_socket_path.string()};
            }
            asio::post(app_->iox_, do_connect);
        }
        else
        {
            asio::post(app_->iox_, do_write);
        }
    };
    auto inject = [&]()
    {
        asio::post(app_->iox_, do_connect);
    };

    configure();
    args_.push_back("--local-path");
    args_.push_back(test_socket_path.string());

    reset();
    EXPECT_NO_THROW(asio::post(app_->iox_, inject));
    EXPECT_NO_THROW(run());
}
