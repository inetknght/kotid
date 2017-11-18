FILE(GLOB KOTI_TCP_TEST_SRC "*.cpp")
FILE(GLOB KOTI_TCP_TEST_HDR "*.hpp")

ADD_EXECUTABLE(koti_tcp_test ${KOTI_TCP_TEST_SRC} ${KOTI_TCP_TEST_HDR})

TARGET_LINK_LIBRARIES(koti_tcp_test
	koti_tcp_lib
	gtest_main
)

GTEST_ADD_TESTS(koti_tcp_test "" ${KOTI_TCP_TEST_SRC})

