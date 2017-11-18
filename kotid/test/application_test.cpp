#include "../application.hpp"

#include "gtest/gtest.h"

#include <memory>
#include <cstdlib>

// This should be defined from the CMakeLists.txt, ADD_DEFINITIONS(-DTEST_DIR_ROOT=...)
#ifndef TEST_DIR_ROOT
#define TEST_DIR_ROOT ""
#endif

class application_test : public ::testing::Test {
public:
	void run() {}
};

TEST_F(application_test, no_parameters) {
    EXPECT_NO_THROW(run());
}
