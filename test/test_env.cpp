#include <string>

#include "glz-util/env.hpp"

#include "catch2/catch_all.hpp"

struct test_struct {
int         number1 = 0;
float       number2 = 0.0f;
std::string message = "";
};

struct env_error_payload {
  std::string env;
  std::string value;
  std::string detail;
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

template <>
struct glz::meta<env_error_payload> {
  using T = env_error_payload;

  static constexpr auto value = glz::object(
    "env", &T::env,
    "value", &T::value,
    "detail", &T::detail
  );
};

TEST_CASE("glaze env") {
  setenv("NUMBER_1", "0", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<test_struct>();

  REQUIRE(result.has_value());
  REQUIRE(result->number1 == 0);
  REQUIRE(result->number2 == 0.0f);
  REQUIRE(result->message == "");
}

TEST_CASE("glaze env number_1") {
  setenv("NUMBER_1", "42", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<test_struct>();

  REQUIRE(result.has_value());
  REQUIRE(result->number1 == 42);
  REQUIRE(result->number2 == 0.0f);
  REQUIRE(result->message == "");
}

TEST_CASE("glaze env json") {
  setenv("NUMBER_1", "0", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<glz::opts{.format=glz::JSON}, test_struct>();

  REQUIRE(result.has_value());
  REQUIRE(result->number1 == 0);
  REQUIRE(result->number2 == 0.0f);
  REQUIRE(result->message == "");
}

TEST_CASE("glaze env json number_1") {
  setenv("NUMBER_1", "42", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<glz::opts{.format=glz::JSON}, test_struct>();

  REQUIRE(result.has_value());
  REQUIRE(result->number1 == 42);
  REQUIRE(result->number2 == 0.0f);
  REQUIRE(result->message == "");
}

TEST_CASE("glaze env invalid number_1") {
  setenv("NUMBER_1", "abc", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<test_struct>();

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().find("\"env\":\"NUMBER_1\"") != std::string::npos);
  REQUIRE(result.error().find("\"detail\":\"") != std::string::npos);
}

TEST_CASE("glaze env number_1 out_of_range") {
  setenv("NUMBER_1", "999999999999999999999999", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<test_struct>();

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().find("\"env\":\"NUMBER_1\"") != std::string::npos);
  REQUIRE(result.error().find("\"detail\":\"") != std::string::npos);
}

TEST_CASE("glaze env json invalid number_1") {
  setenv("NUMBER_1", "abc", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<glz::opts{.format=glz::JSON}, test_struct>();

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().find("\"env\":\"NUMBER_1\"") != std::string::npos);
  REQUIRE(result.error().find("\"detail\":\"") != std::string::npos);
}

TEST_CASE("glaze env error payload can be parsed as json") {
  setenv("NUMBER_1", "abc", 1);
  setenv("NUMBER_2", "0", 1);
  setenv("MESSAGE", "", 1);

  auto const result = glz_util::from_env<test_struct>();

  REQUIRE_FALSE(result.has_value());

  env_error_payload payload{};
  auto const ec = glz::read_json(payload, result.error());

  REQUIRE_FALSE(ec);
  REQUIRE(payload.env == "NUMBER_1");
  REQUIRE(payload.value == "abc");
  REQUIRE_FALSE(payload.detail.empty());
}
