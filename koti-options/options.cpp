#include "options.hpp"

#include "spdlog/spdlog.h"
namespace spd = spdlog;

#include <algorithm>
#include <iostream>
#include <vector>

namespace koti {

options::configuration &
options::get_configuration()
{
	return configuration_;
}

const options::configuration &
options::get_configuration() const
{
	return configuration_;
}

options::descriptions &
options::configurator::operator ()()
{
	return descriptions_;
}

options::configurator_list &
options::configurators()
{
	return configurators_;
}

const options::configurator_list &
options::configurators() const
{
	return configurators_;
}

int
options::commandline_arguments::argc() const
{
	return argc_;
}

char **
options::commandline_arguments::argv() const
{
	return argv_;
}

options &
options::add(configurator & p)
{
	configurators_.push_back(&p);
	return *this;
}

options &
options::operator()(configurator & c)
{
	add(c);
	return *this;
}

options::validate
options::null_configurator::validate_configuration(
	options &
)
{
	return validate::ok;
}

void
options::null_configurator::add_options(
	options &
)
{
}

options::commandline &
options::get_commandline()
{
	return commandline_;
}

const options::commandline &
options::get_commandline() const
{
	return commandline_;
}

options::commandline_arguments::commandline_arguments(
	int argc,
	char **argv
)
	: argc_(argc)
	, argv_(argv)
{
	if ( nullptr != argv_[argc] )
	{
		std::cerr << "warning: argv[" << argc << "] is not null (POSIX violation)\n";
	}
}

options::validate
options::configure()
{
	return configure_internal();
}

const options::commandline_arguments &
options::arguments() const
{
	return args_;
}

options::options(
	const commandline_arguments &args
) :
	args_(args)
{
}

options::validate
options::configure_internal()
{
	using step_description = std::function<validate()>;
	for ( auto step : std::initializer_list<step_description>{
		[&](){return parse_commandline();},
		[&](){return notify_and_validate();}
	})
	{
		auto valid = step();
		switch (valid)
		{
		case validate::reject:
			[[fallthrough]];
		case validate::reject_not_failure:
			return valid;
		case validate::ok:
			break;
		}
	}

	return validate::ok;
}

options::validate
options::parse_commandline()
{
	try
	{
		std::vector<char *> argv(args_.argc(), nullptr);
		std::copy(
			&args_.argv()[0],
			&args_.argv()[args_.argc()],
			argv.begin()
		);

		po::store(
			po::command_line_parser(argv.size(), argv.data())
			.options(commandline_())
			.run(),
			configuration_
		);

		po::notify(configuration_);
	}
	catch (const std::exception & e)
	{
		auto console = spd::stdout_color_mt("console");
		console->error(e.what());

		std::cerr
			<< e.what() << '\n'
			<< '\n'
			<< commandline_()
			<< '\n';
		return validate::reject;
	}

	return validate::ok;
}

options::validate
options::notify_and_validate()
{
	for ( auto * configurator : configurators_ )
	{
		auto valid = configurator->validate_configuration(*this);
		switch (valid)
		{
		case validate::reject:
			[[fallthrough]];
		case validate::reject_not_failure:
			return valid;
		case validate::ok:
			break;
		}
	}

	return validate::ok;
}

} // namespace koti
