#include <iostream>
#include <fmt/core.h>

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  fmt::println("Hello, {}!", "World");
  return 0;
}
