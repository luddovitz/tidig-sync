cmake_minimum_required(VERSION 3.26)
project("tidig_sync (Unix-OSX)")

set(CMAKE_CXX_STANDARD 20)

add_executable(tidig_sync main.cpp)

find_package(CURL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

target_include_directories(tidig_sync PRIVATE ${CURL_INCLUDE_DIRS})
target_link_libraries(tidig_sync PRIVATE ${CURL_LIBRARIES} nlohmann_json::nlohmann_json)