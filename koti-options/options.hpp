#pragma once

#include <vector>
#include <stdexcept>
#include <type_traits>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

namespace koti {

class options
{
public:
	using descriptions = po::options_description;
	using configuration = po::variables_map;

	configuration & get_configuration();
	const configuration & get_configuration() const;

	options() = default;
	options(const options & copy_ctor) = default;
	options(options && move_ctor) = default;
	options & operator=(const options & copy_assign) = default;
	options & operator=(options && move_assign) = default;
	~options() = default;

	enum class validate
	{
		// options were rejected.
		// eg `command --bad-argument` should EXIT_FAILURE
		reject = 0,

		// everything looks dandy
		ok = 1,

		// options were rejected, but not for a failing reason
		// eg `command --help` should not EXIT_FAILURE
		reject_not_failure = 2
	};

	class configurator
	{
	public:
		descriptions & operator()();

		virtual validate validate_configuration(options & storage) = 0;
		virtual void add_options(options & storage) = 0;
	protected:
		descriptions descriptions_;
	};

	using configurator_list = std::vector<configurator*>;

	configurator_list &
	configurators();

	const configurator_list &
	configurators() const;

	options &
	add(configurator & p);

	options &
	operator()(configurator & c);

	class null_configurator
		: public virtual configurator
	{
	public:
		validate validate_configuration(options & storage) override;
		void add_options(options & storage) override;
	};

	class commandline
		: public null_configurator
	{};

	commandline & get_commandline();
	const commandline & get_commandline() const;

	class commandline_arguments
	{
	public:
		commandline_arguments() = default;
		commandline_arguments(const commandline_arguments & copy_ctor) = default;
		commandline_arguments(commandline_arguments && move_ctor) = default;
		commandline_arguments & operator=(const commandline_arguments& copy_assign) = default;
		commandline_arguments & operator=(commandline_arguments && move_assign) = default;
		~commandline_arguments() = default;

		commandline_arguments(int argc, char **argv);

		int argc() const;
		char **argv() const;

	protected:
		int argc_ = 0;
		char **argv_ = nullptr;
	};

	const commandline_arguments & arguments() const;

	options(
		const commandline_arguments & args
	);

	[[nodiscard]]
	validate
	configure();

protected:
	[[nodiscard]]
	validate
	configure_internal();

	[[nodiscard]]
	validate
	parse_commandline();

	[[nodiscard]]
	validate
	notify_and_validate();

	configurator_list configurators_;

	commandline_arguments args_;
	commandline commandline_;

	configuration configuration_;
};

} // namespace koti
