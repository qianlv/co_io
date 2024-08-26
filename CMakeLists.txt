cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if (NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE Release)
endif()

set(COMPILE_FLAGS -Wall -Wextra -Werror -Werror=uninitialized -Werror=return-type -Wno-unused-parameter)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
      set(COMPILE_FLAGS ${COMPILE_FLAGS} -g -ggdb3 -gdwarf-4 -O0)
endif()

add_compile_options(${COMPILE_FLAGS})

project(co_io LANGUAGES CXX)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp")
file(GLOB_RECURSE TESTS "tests/*.cpp")
file(GLOB_RECURSE EXAMPLES "examples/*.cpp")
file(GLOB_RECURSE BENCH "bench/*.cpp")
add_library(co_io STATIC ${SOURCES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

include(FetchContent)
FetchContent_Declare(llhttp
  URL "https://github.com/nodejs/llhttp/archive/refs/tags/release/v9.2.1.tar.gz")
set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
set(BUILD_STATIC_LIBS ON CACHE INTERNAL "")
FetchContent_MakeAvailable(llhttp)
include_directories(${llhttp_SOURCE_DIR}/include)
# Link with the llhttp_static target
target_link_libraries(co_io llhttp_static)

find_package(re2 CONFIG REQUIRED)
target_link_libraries(co_io re2::re2)

macro(add_exec source name)
      add_executable(${name} ${source}/${name}.cpp)
      target_link_libraries(${name} co_io)
      target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endmacro()

macro(add_bench source name)
      add_executable(${name} ${source}/${name}.cpp)
      find_package(benchmark REQUIRED)
      target_link_libraries(${name} benchmark::benchmark)
      target_link_libraries(${name} co_io)
      target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
endmacro()

add_exec(examples echo_server)
add_exec(examples http_parser_test)
add_exec(examples http)
add_exec(examples http_mt)
add_exec(examples ada_radix_tree_test)
add_exec(examples ada_radix_tree_iterator)
add_bench(bench query_sparse_uniform)
add_exec(tests timers_sleep)
add_exec(tests timers_auto_cancel)
add_exec(tests test_when_any)
add_exec(tests test_when_all)
add_exec(tests test_run_task)


add_custom_target(format
      COMMAND clang-format -i ${SOURCES}
      COMMAND clang-format -i ${HEADERS}
      COMMAND clang-format -i ${TESTS}
      COMMAND clang-format -i ${EXAMPLES}
      COMMAND clang-format -i ${BENCH}
)
