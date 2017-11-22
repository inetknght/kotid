
####################################
# Bring Spdlog into the project.

ADD_LIBRARY(spdlog INTERFACE)
TARGET_INCLUDE_DIRECTORIES(spdlog
	INTERFACE 
	spdlog/include
)
TARGET_LINK_LIBRARIES(spdlog
	INTERFACE
	fmt
	${CMAKE_THREAD_LIBS_INIT}
)

# Bring Googletest into the project.
####################################

