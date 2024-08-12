cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if (NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE Release)
endif()

set(COMPILE_FLAGS -Wall -Wextra -Werror -Werror=uninitialized -Werror=return-type -Wno-unused-parameter)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(COMPILE_FLAGS ${COMPILE_FLAGS} -g -ggdb3 -O0)
endif()

add_compile_options(${COMPILE_FLAGS})

project(co_io LANGUAGES CXX)

file(GLOB SOURCES "src/*.cpp")
add_library(co_io STATIC ${SOURCES})

include(FetchContent)
FetchContent_Declare(llhttp
  URL "https://github.com/nodejs/llhttp/archive/refs/tags/release/v8.1.0.tar.gz")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")
FetchContent_MakeAvailable(llhttp)
include_directories(${llhttp_SOURCE_DIR}/include)
# Link with the llhttp_static target
target_link_libraries(co_io llhttp_static)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(co_io spdlog::spdlog spdlog::spdlog_header_only)

macro(add_exec source name)
      add_executable(${name} ${source}/${name}.cpp)
      target_link_libraries(${name} co_io)
      target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endmacro()

add_exec(examples echo_server)
add_exec(examples echo_client)
add_exec(examples timers)
add_exec(examples http_parser_test)
