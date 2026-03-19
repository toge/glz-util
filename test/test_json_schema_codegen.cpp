#include <string>

#include "json_schema_codegen.hpp"

#include "catch2/catch_all.hpp"

TEST_CASE("json schema generator emits flat object structs") {
  auto const schema = R"({
    "type": "object",
    "properties": {
      "name": { "type": "string" },
      "count": { "type": "integer" },
      "enabled": { "type": "boolean" }
    },
    "required": ["name", "count"]
  })";

  auto generated =
    glz_util::schema_codegen::generate_structs_from_json_schema(schema, {.root_name = "AppConfig"});

  REQUIRE(generated.has_value());
  REQUIRE(generated->find("struct AppConfig") != std::string::npos);
  REQUIRE(generated->find("std::string name{};") != std::string::npos);
  REQUIRE(generated->find("std::int64_t count{};") != std::string::npos);
  REQUIRE(generated->find("std::optional<bool> enabled{};") != std::string::npos);
  REQUIRE(generated->find("\"enabled\", &T::enabled") != std::string::npos);
}

TEST_CASE("json schema generator resolves refs and arrays") {
  auto const schema = R"({
    "type": "object",
    "properties": {
      "users": {
        "type": "array",
        "items": { "$ref": "#/$defs/user" }
      }
    },
    "required": ["users"],
    "$defs": {
      "user": {
        "type": "object",
        "properties": {
          "id": { "type": "integer" },
          "name": { "type": "string" }
        },
        "required": ["id", "name"]
      }
    }
  })";

  auto generated =
    glz_util::schema_codegen::generate_structs_from_json_schema(schema, {.root_name = "Directory"});

  REQUIRE(generated.has_value());
  REQUIRE(generated->find("struct User") != std::string::npos);
  REQUIRE(generated->find("std::vector<User> users{};") != std::string::npos);
  REQUIRE(generated->find("\"users\", &T::users") != std::string::npos);
}

TEST_CASE("json schema generator rejects unsupported unions") {
  auto const schema = R"({
    "type": "object",
    "properties": {
      "value": {
        "type": ["string", "integer"]
      }
    }
  })";

  auto generated =
    glz_util::schema_codegen::generate_structs_from_json_schema(schema, {.root_name = "UnionExample"});

  REQUIRE_FALSE(generated.has_value());
  REQUIRE(generated.error().find("union types other than nullable are not supported") != std::string::npos);
}
