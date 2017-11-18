
extern "C" {
// #include <posix.h>
} // extern "C"

#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <boost/asio.hpp>
namespace asio = boost::asio;

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <boost/beast.hpp>
namespace beast = boost::beast;

#include "httpd.hpp"

namespace koti {

class application final {
public:
	class options final {
	public:
		options(int argc = 0, char **argv = nullptr);

		void assign(int argc, char **argv);
	};

	class exit_status {
	public:
		static exit_status success() {
			return {success_};
		}

		static exit_status failure() {
			return {failure_};
		}

		exit_status() = delete;
		exit_status(const exit_status & copy_ctor) = default;
		exit_status & operator=(const exit_status & copy_assign) = default;
		exit_status(exit_status && move_ctor) = default;
		exit_status & operator=(exit_status && move_assign) = default;

		int value() const {
			return exit_status_;
		}

		bool is_success() const {
			return success_ == exit_status_;
		}

		bool is_bad() const {
			return success_ != exit_status_;
		}

		operator bool() const {
			return is_success();
		}

	protected:
		exit_status(int status) : exit_status_(status) {}
		static constexpr const int success_ = EXIT_SUCCESS;
		static constexpr const int failure_ = EXIT_FAILURE;

		int exit_status_ = EXIT_FAILURE;
	};

protected:
	options options_;

public:
	application(options options_ = {}) : options_(options_) {}

	exit_status run();

	asio::io_service ios_;
	std::unique_ptr<asio::io_service::work> work_;

	std::unique_ptr<httpd<>> http_server_;
};

} // namespace koti
