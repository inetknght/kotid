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

	std::unique_ptr<koti::application> app_;
	std::vector<std::string> args_;
	std::vector<const char*> args_raw_;
};

TEST_F(application_test, no_parameters) {
    reset();
    EXPECT_NO_THROW(run());
}

TEST_F(application_test, local_path) {
    std::unique_ptr<int, std::function<int(int*)>> native_(new int, [](int*fd)
    {
        auto result = 0 <= *fd ? ::close(*fd) : 0;
        delete fd;
        return result;
    });
    *native_ = -1;

    int native = ::socket(AF_LOCAL, SOCK_STREAM, 0);
    ASSERT_LE(0, native);

    *native_ = native;

    int result = ::fcntl(native, F_SETFL, O_NONBLOCK);
    ASSERT_LE(0, result);
    result = -1;

    fs::path test_socket_path = koti::test::test_socket_path();

    auto until = std::chrono::system_clock::now() + std::chrono::seconds(1);
    auto check_timeout = [&](const std::string_view & msg)
    {
        if ( until <= std::chrono::system_clock::now() )
        {
            throw koti::exception::timeout{std::string{msg}};
        }
    };

    auto post = [&](auto f){asio::post(app_->iox_, f);};

    std::vector<char>
        native_readbuf, native_writebuf;
    native_readbuf.resize(4096);
    native_writebuf.reserve(4096);

    int
        total_write = 0,
        total_read = 0,
        read_processed = 0;

    std::function<void()> do_check = [&]()
    {
        app_->iox_.stop();
    };

    std::function<void()> do_read = [&]()
    {
        if ( native_readbuf.size() < static_cast<std::size_t>(total_read) )
        {
            throw std::runtime_error{"youdidmorebad"};
        }
        if ( native_readbuf.size() == static_cast<std::size_t>(total_read) )
        {
            post(do_check);
            return;
        }
        result = ::read(native, native_readbuf.data() + total_read, native_readbuf.size() - total_read);
        if ( result == 0 )
        {
            post(do_check);
            return;
        }
        else if ( result < 0 )
        {
            int e = errno;
            switch (e)
            {
            case EAGAIN: [[fallthrough]];
            case EINTR:
                break;
            default:
                throw std::runtime_error{std::string{"unhandled result from ::read(): "} + strerror(e)};
            }
            check_timeout(std::string{"timed out trying to read from "} + test_socket_path.string());
        }
        else
        {
            total_read += result;

            if ( read_processed < total_read )
            {
                if ( read_processed < 4 )
                {
                    if ( total_read < 5 )
                    {
                        post(do_read);
                        return;
                    }
                    auto v = static_cast<std::size_t>(std::min(5, total_read));
                    auto data = std::string_view{native_readbuf.data(), v};
                    if ( data != "HTTP/" )
                    {
                        SCOPED_TRACE("httpd returned non-HTTP response");
                        EXPECT_EQ(data[4], ' ');
                        app_->iox_.stop();
                        return;
                    }
                    read_processed = 5;
                }
            }
        }
        post(do_read);
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
            post(do_read);
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
                break;
            default:
                throw std::runtime_error{std::string{"unhandled result from ::write(): "} + strerror(e)};
            }
            check_timeout(std::string{"timed out trying to write to "} + test_socket_path.string());
        }
        else
        {
            total_write += result;
        }
        post(do_write);
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
            check_timeout(std::string{"timed out trying to connect to "} + test_socket_path.string());
            post(do_connect);
        }
        else
        {
            post(do_write);
        }
    };
    auto inject = [&]()
    {
        post(do_connect);
    };

    configure();
    args_.push_back("--local-path");
    args_.push_back(test_socket_path.string());

    reset();
    EXPECT_NO_THROW(asio::post(app_->iox_, inject));
    EXPECT_NO_THROW(run());
}
