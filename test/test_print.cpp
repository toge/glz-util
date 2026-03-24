#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include "glz-util/print.hpp"

#include "catch2/catch_all.hpp"

struct print_members_meta_struct {
  int         number  = 0;
  std::string message = "";
};

template <>
struct glz::meta<print_members_meta_struct> {
  using T = print_members_meta_struct;

  static constexpr auto value = glz::object(
    "NUMBER", &T::number,
    "MESSAGE", &T::message
  );
};

struct print_members_reflectable_struct {
  int         number  = 0;
  std::string message = "";
};

namespace {

template <typename Fn>
std::string capture_stdout(Fn&& fn) {
  std::ostringstream output;
  auto* const        original = std::cout.rdbuf(output.rdbuf());

  try {
    std::forward<Fn>(fn)();
  } catch (...) {
    std::cout.rdbuf(original);
    throw;
  }

  std::cout.rdbuf(original);
  return output.str();
}

}  // namespace

TEST_CASE("print members uses glaze meta keys") {
  auto const value = print_members_meta_struct{.number = 42, .message = "hello"};

  auto const output = capture_stdout([&]() {
    glz_util::print_members(value);
  });

  REQUIRE(output == "0 NUMBER 42\n1 MESSAGE hello\n");
}

TEST_CASE("print members uses reflected member names") {
  auto const value = print_members_reflectable_struct{.number = 7, .message = "world"};

  auto const output = capture_stdout([&]() {
    glz_util::print_members(value);
  });

  REQUIRE(output == "0 number 7\n1 message world\n");
}
