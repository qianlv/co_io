cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if (NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE Release)
endif()

set(COMPILE_FLAGS -Wall -Wextra -Wpedantic -Werror -Werror=uninitialized
      -Werror=return-type
      -Wconversion
      -Werror=unused-result
      -Werror=suggest-override
      -Wzero-as-null-pointer-constant
      -Wmissing-declarations
      -Wold-style-cast
      -Wnon-virtual-dtor
      -Wcast-align
      -Wdouble-promotion)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(COMPILE_FLAGS ${COMPILE_FLAGS} -g -ggdb3 -O0)
endif()

add_compile_options(${COMPILE_FLAGS})


project(co_io LANGUAGES CXX)

find_package(spdlog CONFIG REQUIRED)

file(GLOB SOURCES "src/*.cpp")
add_library(co_io STATIC ${SOURCES})
target_link_libraries(co_io spdlog::spdlog spdlog::spdlog_header_only)

add_executable(echo_server "examples/echo_server.cpp")
target_link_libraries(echo_server co_io)
target_include_directories(echo_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(echo_client "examples/echo_client.cpp")
target_link_libraries(echo_client co_io)
target_include_directories(echo_client PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(timers "examples/timers.cpp")
target_link_libraries(timers co_io)
target_include_directories(timers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# add_executable(test_task "examples/test_task.cpp")
# target_link_libraries(test_task co_io)
# target_include_directories(test_task PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
