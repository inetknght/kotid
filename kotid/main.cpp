#include "application.hpp"

#include <cstdlib>
#include <iostream>

int main(
	int argc,
	char **argv
)
{
	try {
		return koti::application{{argc, argv}}.run().value();
	} catch (const std::exception & e) {
		std::cerr << "uncaught exception: " << e.what() << '\n';
	}
	return EXIT_FAILURE;
}
