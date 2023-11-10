cmake_minimum_required(VERSION 3.26)
project(tidig_sync)

set(CMAKE_CXX_STANDARD 20)

add_executable(main.cpp)

find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(tidig_sync ${CURL_LIBRARIES} nlohmann_json::nlohmann_json)