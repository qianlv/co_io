cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER ${CMAKE_CXX_COMPILER})

project(co_io LANGUAGES CXX)

add_compile_options(-Wall -Wextra -Wpedantic -Werror -Werror=uninitialized
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

find_package(fmt CONFIG REQUIRED)

file(GLOB SOURCES "src/*.cpp")
add_library(co_io STATIC ${SOURCES})
target_link_libraries(co_io fmt::fmt)

add_executable(echo_server "examples/echo_server.cpp")
target_link_libraries(echo_server co_io)
