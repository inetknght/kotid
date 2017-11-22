CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

PROJECT(kotid CXX)

# If CMAKE_BUILD_TYPE is not configured, then set to default
# {None,Debug,RelWithDebInfo,Release,MinSizeRel}
IF (NOT CMAKE_BUILD_TYPE)
	SET (CMAKE_BUILD_TYPE "RelWithDebInfo")
ENDIF()
MESSAGE (STATUS "Current build type is ${CMAKE_BUILD_TYPE}")

# Must find threads. We don't specifically use threading in our model, but
# do use some supporting features of it
FIND_PACKAGE(Threads REQUIRED)

# Enable cmake test features
enable_testing()
INCLUDE(FindGTest)

# Enable GetGitRevisionDescription module
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
git_describe(GIT_DESCRIBE --tags --long)
string(REGEX REPLACE "v([0-9]+)\\.([0-9]+)\\.([0-9]+)-([0-9a-z]+)" "\\1.\\2.\\3" VERSION "${GIT_DESCRIBE}")

message(STATUS "Refspec: ${GIT_REFSPEC} Sha1: ${GIT_SHA1}")
message(STATUS "Describe: ${GIT_DESCRIBE}")
message(STATUS "Version: ${VERSION}")

set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "koti")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "Keith Bennett <koti@knightoftheinter.net>")
set(CPACK_PACKAGE_VENDOR "Knight of the Internet")
set(CPACK_PACKAGE_RELOCATABLE ON)
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)

IF (COMMAND cmake_policy)
	# Be more explicit about link library search paths
	cmake_policy(SET CMP0003 NEW)
ENDIF()

# Find Boost for subprojects
SET(Boost_USE_STATIC_LIBS ON)
SET(Boost_USE_MULTITHREADED ON)
SET(Boost_USE_STATIC_RUNTIME OFF)
FIND_PACKAGE(Boost 1.62.0 REQUIRED COMPONENTS program_options system filesystem iostreams)
INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIRS})

# Address Sanitizer - available in clang 3.2+ and gcc 4.8+
# Remember, you can turn on LEAK_SANTIZIER by setting environment variable ASAN_OPTIONS=detect_leaks=1 when running executable.
OPTION(ASAN_BUILD "When on, we will build for and link against Address Sanitizer" off)
IF(ASAN_BUILD)
        MESSAGE(STATUS "Address Sanitizer libraries selected")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
        SET(LINK_FLAGS "${LINK_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
ENDIF()

FIND_PACKAGE(Threads)

ADD_SUBDIRECTORY(thirdparty)
INCLUDE_DIRECTORIES(SYSTEM ${GTEST_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/beast)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED 17)
SET(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror=return-type -Werror=shift-count-overflow -Werror=shift-negative-value -Werror=shift-overflow -Wno-long-long -pedantic -Wno-unused-function -Wno-vla -Wswitch-enum -march=native -mtune=native")

# Find and set up zlib (anything using zlib needs to TARGET_LINK_LIBRARIES(target ${ZLIB_LIRARIES})
FIND_PACKAGE(ZLIB)
INCLUDE_DIRECTORIES(SYSTEM ${ZLIB_INCLUDE_DIRS})

# Find and set up bzip2 (anything using bzip2 needs to TARGET_LINK_LIBRARIES(target ${BZIP2_LIBRARIES})
FIND_PACKAGE(BZip2 REQUIRED)
INCLUDE_DIRECTORIES(SYSTEM ${BZIP2_INCLUDE_DIRS})

# kotipp; a support library providing syntactic sugar for C++
ADD_SUBDIRECTORY(kotipp)

ADD_SUBDIRECTORY(koti-options)
ADD_SUBDIRECTORY(koti-tcp)
ADD_SUBDIRECTORY(koti-httpd)
ADD_SUBDIRECTORY(kotid)

