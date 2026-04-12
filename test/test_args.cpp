#include <array>
#include <string>
#include <vector>

#include "glaze/glaze.hpp"

#if __has_include("glz-util/args.hpp")
#include "glz-util/args.hpp"
#define GLZ_UTIL_HAS_ARGS_HPP 1
#else
#define GLZ_UTIL_HAS_ARGS_HPP 0
#endif

#include "catch2/catch_all.hpp"

struct args_test_struct {
  int         port = 0;
  float       ratio = 0.0f;
  std::string message = "";
  bool        enabled = false;
};

struct args_json_struct {
  std::vector<int> numbers{};
  std::string      message{};
};

struct args_error_payload {
  std::string key;
  std::string value;
  std::string detail;
};

template <>
struct glz::meta<args_test_struct> {
  using T = args_test_struct;

  static constexpr auto value = glz::object(
    "port", &T::port,
    "ratio", &T::ratio,
    "message", &T::message,
    "enabled", &T::enabled
  );
};

template <>
struct glz::meta<args_json_struct> {
  using T = args_json_struct;

  static constexpr auto value = glz::object(
    "numbers", &T::numbers,
    "message", &T::message
  );
};

template <>
struct glz::meta<args_error_payload> {
  using T = args_error_payload;

  static constexpr auto value = glz::object(
    "key", &T::key,
    "value", &T::value,
    "detail", &T::detail
  );
};

TEST_CASE("glaze args header exists") {
  REQUIRE(GLZ_UTIL_HAS_ARGS_HPP == 1);
}

#if GLZ_UTIL_HAS_ARGS_HPP

TEST_CASE("glaze args parses equals form case-insensitively") {
  auto constexpr argv = std::array{
    "app",
    "--PORT=8080",
    "--ratio=0.5",
    "--Message=hello",
    "--ENABLED=true"
  };

  auto const result = glz_util::from_args<args_test_struct>(static_cast<int>(argv.size()), argv.data());

  REQUIRE(result.has_value());
  REQUIRE(result->port == 8080);
  REQUIRE(result->ratio == Catch::Approx(0.5f));
  REQUIRE(result->message == "hello");
  REQUIRE(result->enabled);
}

TEST_CASE("glaze args parses space separated single dash form") {
  auto constexpr argv = std::array{
    "app",
    "-port", "42",
    "-ratio", "1.25",
    "-message", "world",
    "-enabled", "false"
  };

  auto const result = glz_util::from_args<args_test_struct>(static_cast<int>(argv.size()), argv.data());

  REQUIRE(result.has_value());
  REQUIRE(result->port == 42);
  REQUIRE(result->ratio == Catch::Approx(1.25f));
  REQUIRE(result->message == "world");
  REQUIRE_FALSE(result->enabled);
}

TEST_CASE("glaze args ignores unknown keys and keeps last value") {
  auto constexpr argv = std::array{
    "app",
    "--unknown=123",
    "--port=10",
    "--PORT=12",
    "--ratio", "2.5",
    "--message", "ok",
    "--enabled", "true"
  };

  auto const result = glz_util::from_args<args_test_struct>(static_cast<int>(argv.size()), argv.data());

  REQUIRE(result.has_value());
  REQUIRE(result->port == 12);
  REQUIRE(result->ratio == Catch::Approx(2.5f));
  REQUIRE(result->message == "ok");
  REQUIRE(result->enabled);
}

TEST_CASE("glaze args returns parse_error payload on invalid value") {
  auto constexpr argv = std::array{
    "app",
    "--port=abc"
  };

  auto const result = glz_util::from_args<args_test_struct>(static_cast<int>(argv.size()), argv.data());

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().kind == glz_util::ArgsErrorKind::parse_error);

  args_error_payload payload{};
  auto const ec = glz::read_json(payload, result.error().message);

  REQUIRE_FALSE(ec);
  REQUIRE(payload.key == "port");
  REQUIRE(payload.value == "abc");
  REQUIRE_FALSE(payload.detail.empty());
}

TEST_CASE("glaze args json opts reads json literals") {
  auto constexpr argv = std::array{
    "app",
    "--numbers=[1,2,3]",
    "--message=\"hello json\""
  };

  auto const result = glz_util::from_args<glz::opts{.format = glz::JSON}, args_json_struct>(
    static_cast<int>(argv.size()), argv.data()
  );

  REQUIRE(result.has_value());
  REQUIRE(result->numbers == std::vector<int>{1, 2, 3});
  REQUIRE(result->message == "hello json");
}

TEST_CASE("glaze args reports help_requested for long help flag") {
  auto constexpr argv = std::array{
    "app",
    "--help"
  };

  auto const result = glz_util::from_args<args_test_struct>(static_cast<int>(argv.size()), argv.data());

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().kind == glz_util::ArgsErrorKind::help_requested);
  REQUIRE(result.error().message == "help requested");
  REQUIRE(result.error().is_help_requested());
}

TEST_CASE("glaze args reports help_requested for short help flag") {
  auto constexpr argv = std::array{
    "app",
    "-h"
  };

  auto const result = glz_util::from_args<args_test_struct>(static_cast<int>(argv.size()), argv.data());

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().kind == glz_util::ArgsErrorKind::help_requested);
  REQUIRE(result.error().message == "help requested");
}

TEST_CASE("glaze args help short-circuits parse errors") {
  auto constexpr argv = std::array{
    "app",
    "--port=abc",
    "--help"
  };

  auto const result = glz_util::from_args<args_test_struct>(static_cast<int>(argv.size()), argv.data());

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().kind == glz_util::ArgsErrorKind::help_requested);
  REQUIRE(result.error().message == "help requested");
}

TEST_CASE("glaze args json opts reports help_requested") {
  auto constexpr argv = std::array{
    "app",
    "-h",
    "--numbers=[1,2,3]"
  };

  auto const result = glz_util::from_args<glz::opts{.format = glz::JSON}, args_json_struct>(
    static_cast<int>(argv.size()), argv.data()
  );

  REQUIRE_FALSE(result.has_value());
  REQUIRE(result.error().kind == glz_util::ArgsErrorKind::help_requested);
  REQUIRE(result.error().message == "help requested");
}

#endif
