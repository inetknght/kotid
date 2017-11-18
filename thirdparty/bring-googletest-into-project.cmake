
####################################
# Bring Googletest into the project.

# Just using gtest for now, not gmock.
# Maybe in the future this might change...
SET(BUILD_GMOCK OFF)
SET(BUILD_GTEST ON)

# I wish we could just do:
# IMPORT_TARGETS((OPTIONAL GIT TARGET REVISION(s)) (SUBPROJECT ROOT CMAKELISTS FILE(s)) (DESIRED TARGETS/INFORMATION/ETC))
# alas, I can only wish...?

# I wish CMake would automatically configure this per-export
SET(GTEST_EXPORT_CMAKE_TARGETS "ON:${CMAKE_BINARY_DIR}/googletest.cmake" CACHE PATH "Path to googletest cmake import file")
ExternalProject_Add(
        googletest-distribution
        # This project is using git submodules, so use that to checkout the git repository.
        GIT_REPOSITORY "${CMAKE_CURRENT_SOURCE_DIR}/googletest"
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
)

# I wish we could just do this (maybe point to git alias and project root cmake path and target(s))
# Wait, does this work?
ADD_LIBRARY(gtest STATIC IMPORTED)
ADD_LIBRARY(gtest_main STATIC IMPORTED)

# Uh, do I need to set this?
SET(GTEST_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/googletest/include" PARENT_SCOPE)


# Bring Googletest into the project.
####################################

