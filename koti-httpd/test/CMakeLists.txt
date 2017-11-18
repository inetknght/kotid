FILE(GLOB KOTI_HTTPD_TEST_SRC "*.cpp")
FILE(GLOB KOTI_HTTPD_TEST_HDR "*.hpp")

ADD_EXECUTABLE(koti_httpd_test ${KOTI_HTTPD_TEST_SRC} ${KOTI_HTTPD_TEST_HDR})

TARGET_LINK_LIBRARIES(koti_httpd_test
	koti_httpd_lib
	gtest_main
)

GTEST_ADD_TESTS(koti_httpd_test "" ${KOTI_HTTPD_TEST_SRC})

