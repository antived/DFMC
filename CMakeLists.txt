cmake_minimum_required(VERSION 3.10)
project(RemoteServerProject)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/rapidjson/include
    ${CMAKE_SOURCE_DIR}/yaml-cpp/include
    ${CMAKE_SOURCE_DIR}/spdlog/include
    ${CMAKE_SOURCE_DIR}/cpp-httplib
    ${CMAKE_SOURCE_DIR}/central_server
)

find_package(yaml-cpp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)
include_directories(${PQXX_INCLUDE_DIRS})

add_executable(remote_runner
    remote_runner.cpp
    remote_server.cpp
    parser_with_yaml.cpp
)
target_link_libraries(remote_runner
    yaml-cpp
    pthread
    ${PQXX_LIBRARIES}
)
add_executable(central_server
    central_server/api/endpoints.cpp
    central_server/main.cpp
)
target_link_libraries(central_server
    yaml-cpp
    pthread
    ${PQXX_LIBRARIES}
)
