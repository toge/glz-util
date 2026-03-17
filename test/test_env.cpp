#include <string>

#include "glz_util/env.hpp"

#include "catch2/catch_all.hpp"

struct test_struct {
int         number1 = 0;
float       number2 = 0.0f;
std::string message = "";
};

template <>
struct glz::meta<test_struct> {
  using T  = test_struct;

  static constexpr auto value = glz::object(
    "NUMBER_1", &T::number1,
    "NUMBER_2", &T::number2,
    "MESSAGE", &T::message
);
};

TEST_CASE("glaze env") {
  setenv("NUMBER_1", "0", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<test_struct>();

  REQUIRE(result.number1 == 0);
  REQUIRE(result.number2 == 0.0f);
  REQUIRE(result.message == "");
}

TEST_CASE("glaze env number_1") {
  setenv("NUMBER_1", "42", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<test_struct>();

  REQUIRE(result.number1 == 42);
  REQUIRE(result.number2 == 0.0f);
  REQUIRE(result.message == "");
}

TEST_CASE("glaze env json") {
  setenv("NUMBER_1", "0", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<glz::opts{.format=glz::JSON}, test_struct>();

  REQUIRE(result.number1 == 0);
  REQUIRE(result.number2 == 0.0f);
  REQUIRE(result.message == "");
}

TEST_CASE("glaze env json number_1") {
  setenv("NUMBER_1", "42", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<glz::opts{.format=glz::JSON}, test_struct>();

  REQUIRE(result.number1 == 42);
  REQUIRE(result.number2 == 0.0f);
  REQUIRE(result.message == "");
}
