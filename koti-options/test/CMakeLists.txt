FILE(GLOB KOTI_OPTIONS_TEST_SRC "*.cpp")
FILE(GLOB KOTI_OPTIONS_TEST_HDR "*.hpp")

ADD_EXECUTABLE(koti_options_test ${KOTI_OPTIONS_TEST_SRC} ${KOTI_OPTIONS_TEST_HDR})

TARGET_LINK_LIBRARIES(koti_options_test
	koti_options_lib
	gtest_main
)

GTEST_ADD_TESTS(koti_options_test "" ${KOTI_OPTIONS_TEST_SRC})

