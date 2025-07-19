cmake_minimum_required(VERSION 3.10)
project(RemoteServerProject)

set(CMAKE_CXX_STANDARD 17)
include_directories(
	${CMAKE_SOURCE_DIR}
    	${CMAKE_SOURCE_DIR}/rapidjson/include
    	${CMAKE_SOURCE_DIR}/yaml-cpp/include
    	${CMAKE_SOURCE_DIR}/spdlog/include
    	${CMAKE_SOURCE_DIR}/cpp-httplib
)
add_executable(remote_runner
	remote_runner.cpp
	remote_server.cpp
	parser_with_yaml.cpp
)
#we need to link the yaml-cppp and pthread (for thrading).
find_package(yaml-cpp REQUIRED)
target_link_libraries(remote_runner yaml-cpp pthread)
