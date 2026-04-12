#include <optional>
#include <string>
#include <vector>

#include "glaze/glaze.hpp"

#if __has_include("glz-util/diff.hpp")
#include "glz-util/diff.hpp"
#define GLZ_UTIL_HAS_DIFF_HPP 1
#else
#define GLZ_UTIL_HAS_DIFF_HPP 0
#endif

#include "catch2/catch_all.hpp"

struct diff_test_struct {
  int                number = 0;
  std::string        message{};
  std::optional<int> retry{};
  std::vector<int>   values{};
};

template <>
struct glz::meta<diff_test_struct> {
  using T = diff_test_struct;

  static constexpr auto value = glz::object(
    "NUMBER", &T::number,
    "MESSAGE", &T::message,
    "RETRY", &T::retry,
    "VALUES", &T::values
  );
};

TEST_CASE("glaze diff header exists") {
  REQUIRE(GLZ_UTIL_HAS_DIFF_HPP == 1);
}

#if GLZ_UTIL_HAS_DIFF_HPP

TEST_CASE("glaze diff returns empty vector when all fields match") {
  auto const before = diff_test_struct{.number = 1, .message = "same", .retry = 5, .values = {1, 2}};
  auto const after = diff_test_struct{.number = 1, .message = "same", .retry = 5, .values = {1, 2}};

  auto const diffs = glz_util::diff_members(before, after);

  REQUIRE(diffs.empty());
}

TEST_CASE("glaze diff returns one entry for one changed field") {
  auto const before = diff_test_struct{.number = 30, .message = "localhost", .retry = 1, .values = {1, 2}};
  auto const after = diff_test_struct{.number = 60, .message = "localhost", .retry = 1, .values = {1, 2}};

  auto const diffs = glz_util::diff_members(before, after);

  REQUIRE(diffs.size() == 1);
  REQUIRE(diffs[0].key == "NUMBER");
  REQUIRE(diffs[0].before == "30");
  REQUIRE(diffs[0].after == "60");
}

TEST_CASE("glaze diff returns all changed fields in reflected order") {
  auto const before = diff_test_struct{.number = 1, .message = "old", .retry = std::nullopt, .values = {1, 2}};
  auto const after = diff_test_struct{.number = 2, .message = "new", .retry = 7, .values = {3, 4, 5}};

  auto const diffs = glz_util::diff_members(before, after);

  REQUIRE(diffs.size() == 4);
  REQUIRE(diffs[0].key == "NUMBER");
  REQUIRE(diffs[0].before == "1");
  REQUIRE(diffs[0].after == "2");
  REQUIRE(diffs[1].key == "MESSAGE");
  REQUIRE(diffs[1].before == "\"old\"");
  REQUIRE(diffs[1].after == "\"new\"");
  REQUIRE(diffs[2].key == "RETRY");
  REQUIRE(diffs[2].before == "null");
  REQUIRE(diffs[2].after == "7");
  REQUIRE(diffs[3].key == "VALUES");
  REQUIRE(diffs[3].before == "[1,2]");
  REQUIRE(diffs[3].after == "[3,4,5]");
}

TEST_CASE("glaze diff detects optional field changes") {
  auto const before = diff_test_struct{.number = 1, .message = "same", .retry = std::nullopt, .values = {1}};
  auto const after = diff_test_struct{.number = 1, .message = "same", .retry = 42, .values = {1}};

  auto const diffs = glz_util::diff_members(before, after);

  REQUIRE(diffs.size() == 1);
  REQUIRE(diffs[0].key == "RETRY");
  REQUIRE(diffs[0].before == "null");
  REQUIRE(diffs[0].after == "42");
}

TEST_CASE("glaze diff detects vector field changes") {
  auto const before = diff_test_struct{.number = 1, .message = "same", .retry = 2, .values = {1, 2, 3}};
  auto const after = diff_test_struct{.number = 1, .message = "same", .retry = 2, .values = {1, 2, 4}};

  auto const diffs = glz_util::diff_members(before, after);

  REQUIRE(diffs.size() == 1);
  REQUIRE(diffs[0].key == "VALUES");
  REQUIRE(diffs[0].before == "[1,2,3]");
  REQUIRE(diffs[0].after == "[1,2,4]");
}

TEST_CASE("glaze diff field diff list can be written as json") {
  auto const diffs = std::vector<glz_util::FieldDiff>{
    {.key = "NUMBER", .before = "1", .after = "2"},
    {.key = "MESSAGE", .before = "\"old\"", .after = "\"new\""}
  };

  auto output = std::string{};
  auto const ec = glz::write_json(diffs, output);

  REQUIRE_FALSE(ec);
  REQUIRE(output == R"([{"key":"NUMBER","before":"1","after":"2"},{"key":"MESSAGE","before":"\"old\"","after":"\"new\""}])");
}

#endif
