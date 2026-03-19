#ifndef __GLZ_UTIL_JSON_SCHEMA_CODEGEN__
#define __GLZ_UTIL_JSON_SCHEMA_CODEGEN__

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "glaze/glaze.hpp"

namespace glz_util::schema_codegen {

struct codegen_options {
  std::string root_name = "";
};

namespace internal {

using json_value = glz::generic_json<glz::num_mode::u64>;
using json_array = json_value::array_t;
using json_object = json_value::object_t;

struct field_definition {
  std::string json_name;
  std::string member_name;
  std::string cpp_type;
};

struct struct_definition {
  std::string                  type_name;
  std::vector<field_definition> fields;
};

struct resolved_schema {
  json_value const* node = nullptr;
  std::string       path;
};

inline auto make_error(std::string_view path, std::string_view message) -> std::string {
  std::string error;
  error.reserve(path.size() + message.size() + 3);
  error.append("[");
  error.append(path);
  error.append("] ");
  error.append(message);
  return error;
}

inline auto get_object(json_value const& value) -> json_object const* {
  return value.template get_if<json_object>();
}

inline auto get_array(json_value const& value) -> json_array const* {
  return value.template get_if<json_array>();
}

inline auto get_string(json_value const& value) -> std::string const* {
  return value.template get_if<std::string>();
}

inline auto get_bool(json_value const& value) -> bool const* {
  return value.template get_if<bool>();
}

inline auto find_member(json_value const& value, std::string_view key) -> json_value const* {
  auto const* object = get_object(value);
  if (object == nullptr) {
    return nullptr;
  }

  auto const iter = object->find(key);
  if (iter == object->end()) {
    return nullptr;
  }

  return &iter->second;
}

inline auto is_cpp_keyword(std::string_view value) -> bool {
  static constexpr std::string_view keywords[] = {
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break",
    "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept",
    "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await",
    "co_return", "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast",
    "else", "enum", "explicit", "export", "extern", "false", "float", "for", "friend", "goto",
    "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not",
    "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
    "register", "reinterpret_cast", "requires", "return", "short", "signed", "sizeof",
    "static", "static_assert", "static_cast", "struct", "switch", "template", "this",
    "thread_local", "throw", "true", "try", "typedef", "typeid", "typename", "union",
    "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
  };

  return std::ranges::find(keywords, value) != std::end(keywords);
}

inline auto sanitize_member_name(std::string_view input) -> std::string {
  std::string result;
  result.reserve(input.size() + 1);

  bool previous_was_separator = false;
  for (char ch : input) {
    auto const uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch) != 0) {
      if (result.empty() && std::isdigit(uch) != 0) {
        result.push_back('_');
      }
      result.push_back(static_cast<char>(std::tolower(uch)));
      previous_was_separator = false;
    } else if (!result.empty() && !previous_was_separator) {
      result.push_back('_');
      previous_was_separator = true;
    }
  }

  while (!result.empty() && result.back() == '_') {
    result.pop_back();
  }

  if (result.empty()) {
    result = "field";
  }

  if (is_cpp_keyword(result)) {
    result.push_back('_');
  }

  return result;
}

inline auto to_pascal_case(std::string_view input) -> std::string {
  std::string result;
  result.reserve(input.size() + 6);

  bool uppercase_next = true;
  for (char ch : input) {
    auto const uch = static_cast<unsigned char>(ch);
    if (std::isalnum(uch) != 0) {
      if (result.empty() && std::isdigit(uch) != 0) {
        result.append("Schema");
      }

      if (uppercase_next) {
        result.push_back(static_cast<char>(std::toupper(uch)));
        uppercase_next = false;
      } else {
        result.push_back(ch);
      }
    } else {
      uppercase_next = true;
    }
  }

  if (result.empty()) {
    result = "GeneratedObject";
  }

  if (is_cpp_keyword(result)) {
    result.push_back('_');
  }

  return result;
}

inline auto reserve_unique_name(std::unordered_set<std::string>& used_names, std::string base_name) -> std::string {
  if (base_name.empty()) {
    base_name = "GeneratedObject";
  }

  auto candidate = base_name;
  std::size_t suffix = 2;
  while (!used_names.insert(candidate).second) {
    candidate = base_name + std::to_string(suffix);
    ++suffix;
  }
  return candidate;
}

inline auto wrap_optional_once(std::string type_name) -> std::string {
  if (type_name.starts_with("std::optional<")) {
    return type_name;
  }
  return "std::optional<" + type_name + ">";
}

inline auto decode_json_pointer_token(std::string_view token) -> std::string {
  std::string decoded;
  decoded.reserve(token.size());

  for (std::size_t i = 0; i < token.size(); ++i) {
    if (token[i] == '~' && (i + 1) < token.size()) {
      if (token[i + 1] == '0') {
        decoded.push_back('~');
        ++i;
        continue;
      }
      if (token[i + 1] == '1') {
        decoded.push_back('/');
        ++i;
        continue;
      }
    }
    decoded.push_back(token[i]);
  }

  return decoded;
}

inline auto encode_json_pointer_token(std::string_view token) -> std::string {
  std::string encoded;
  encoded.reserve(token.size());

  for (char ch : token) {
    if (ch == '~') {
      encoded.append("~0");
    } else if (ch == '/') {
      encoded.append("~1");
    } else {
      encoded.push_back(ch);
    }
  }

  return encoded;
}

inline auto last_pointer_token(std::string_view pointer) -> std::string {
  auto const pos = pointer.find_last_of('/');
  if (pos == std::string_view::npos || pos + 1 >= pointer.size()) {
    return {};
  }
  return decode_json_pointer_token(pointer.substr(pos + 1));
}

inline auto collect_type_names(json_value const& schema, std::string_view path)
  -> std::expected<std::vector<std::string>, std::string> {
  auto const* type_node = find_member(schema, "type");
  if (type_node == nullptr) {
    return std::vector<std::string>{};
  }

  if (auto const* type_name = get_string(*type_node); type_name != nullptr) {
    return std::vector<std::string>{*type_name};
  }

  auto const* type_array = get_array(*type_node);
  if (type_array == nullptr) {
    return std::unexpected(make_error(path, "`type` must be a string or array"));
  }

  std::vector<std::string> result;
  result.reserve(type_array->size());
  for (auto const& entry : *type_array) {
    auto const* type_name = get_string(entry);
    if (type_name == nullptr) {
      return std::unexpected(make_error(path, "`type` array entries must be strings"));
    }
    result.push_back(*type_name);
  }

  return result;
}

inline auto infer_enum_type(json_value const& schema, std::string_view path) -> std::expected<std::string, std::string> {
  auto const* enum_node = find_member(schema, "enum");
  if (enum_node == nullptr) {
    return std::unexpected(make_error(path, "schema type is missing"));
  }

  auto const* values = get_array(*enum_node);
  if (values == nullptr || values->empty()) {
    return std::unexpected(make_error(path, "`enum` must be a non-empty array"));
  }

  auto const& first = values->front();
  if (get_string(first) != nullptr) {
    return std::string{"string"};
  }
  if (first.template get_if<uint64_t>() != nullptr || first.template get_if<int64_t>() != nullptr) {
    return std::string{"integer"};
  }
  if (first.template get_if<double>() != nullptr) {
    return std::string{"number"};
  }
  if (get_bool(first) != nullptr) {
    return std::string{"boolean"};
  }

  return std::unexpected(make_error(path, "unsupported `enum` value type"));
}

class generator {
 public:
  generator(json_value const& schema, codegen_options options)
      : schema_(schema), options_(std::move(options)) {}

  auto generate() -> std::expected<std::string, std::string> {
    auto const root_name_hint = !options_.root_name.empty() ? options_.root_name : std::string{"GeneratedRoot"};
    auto root_type = resolve_type(schema_, root_name_hint, "#");
    if (!root_type) {
      return std::unexpected(std::move(root_type.error()));
    }

    if (*root_type != path_to_type_["#"]) {
      return std::unexpected(make_error("#", "top-level schema must describe an object"));
    }

    std::ostringstream output;
    output << "#pragma once\n\n";
    output << "#include <cstdint>\n";
    output << "#include <optional>\n";
    output << "#include <string>\n";
    output << "#include <vector>\n\n";
    output << "#include \"glaze/glaze.hpp\"\n\n";

    for (auto const& definition : definitions_) {
      output << "struct " << definition.type_name << " {\n";
      for (auto const& field : definition.fields) {
        output << "  " << field.cpp_type << " " << field.member_name << "{};\n";
      }
      output << "};\n\n";

      output << "template <>\n";
      output << "struct glz::meta<" << definition.type_name << "> {\n";
      output << "  using T = " << definition.type_name << ";\n\n";

      if (definition.fields.empty()) {
        output << "  static constexpr auto value = glz::object();\n";
      } else {
        output << "  static constexpr auto value = glz::object(\n";
        for (std::size_t index = 0; index < definition.fields.size(); ++index) {
          auto const& field = definition.fields[index];
          output << "    \"" << field.json_name << "\", &T::" << field.member_name;
          if (index + 1 < definition.fields.size()) {
            output << ",";
          }
          output << "\n";
        }
        output << "  );\n";
      }

      output << "};\n\n";
    }

    return output.str();
  }

 private:
  auto resolve_type(json_value const& schema, std::string_view name_hint, std::string const& path)
    -> std::expected<std::string, std::string> {
    if (find_member(schema, "oneOf") != nullptr || find_member(schema, "anyOf") != nullptr ||
        find_member(schema, "allOf") != nullptr) {
      return std::unexpected(make_error(path, "`oneOf`, `anyOf`, and `allOf` are not supported"));
    }

    if (auto const* additional_properties = find_member(schema, "additionalProperties");
        additional_properties != nullptr && get_object(*additional_properties) != nullptr) {
      return std::unexpected(make_error(path, "schema-valued `additionalProperties` is not supported"));
    }

    if (auto const* ref_node = find_member(schema, "$ref"); ref_node != nullptr) {
      auto const* ref_value = get_string(*ref_node);
      if (ref_value == nullptr) {
        return std::unexpected(make_error(path, "`$ref` must be a string"));
      }

      auto resolved = resolve_ref(*ref_value);
      if (!resolved) {
        return std::unexpected(std::move(resolved.error()));
      }

      auto const ref_name_hint = !last_pointer_token(resolved->path).empty()
                                   ? last_pointer_token(resolved->path)
                                   : std::string{name_hint};
      return resolve_type(*resolved->node, ref_name_hint, resolved->path);
    }

    auto type_names = collect_type_names(schema, path);
    if (!type_names) {
      return std::unexpected(std::move(type_names.error()));
    }

    bool nullable = false;
    std::vector<std::string> filtered_types;
    filtered_types.reserve(type_names->size());
    for (auto const& type_name : *type_names) {
      if (type_name == "null") {
        nullable = true;
      } else {
        filtered_types.push_back(type_name);
      }
    }

    std::string schema_type;
    if (filtered_types.empty()) {
      if (find_member(schema, "properties") != nullptr || find_member(schema, "required") != nullptr) {
        schema_type = "object";
      } else if (find_member(schema, "items") != nullptr) {
        schema_type = "array";
      } else {
        auto inferred = infer_enum_type(schema, path);
        if (!inferred) {
          return std::unexpected(std::move(inferred.error()));
        }
        schema_type = std::move(*inferred);
      }
    } else if (filtered_types.size() == 1) {
      schema_type = filtered_types.front();
    } else {
      return std::unexpected(make_error(path, "union types other than nullable are not supported"));
    }

    auto result = [&]() -> std::expected<std::string, std::string> {
      if (schema_type == "object") {
        return ensure_struct(schema, choose_struct_name(schema, name_hint, path), path);
      }
      if (schema_type == "array") {
        auto const* items = find_member(schema, "items");
        if (items == nullptr) {
          return std::unexpected(make_error(path, "`array` schema requires `items`"));
        }

        auto element_type = resolve_type(*items, choose_nested_name(name_hint, "item"), path + "/items");
        if (!element_type) {
          return std::unexpected(std::move(element_type.error()));
        }
        return std::string{"std::vector<"} + *element_type + ">";
      }
      if (schema_type == "string") {
        return std::string{"std::string"};
      }
      if (schema_type == "integer") {
        return std::string{"std::int64_t"};
      }
      if (schema_type == "number") {
        return std::string{"double"};
      }
      if (schema_type == "boolean") {
        return std::string{"bool"};
      }

      return std::unexpected(make_error(path, "unsupported schema type: " + schema_type));
    }();

    if (!result) {
      return result;
    }

    if (nullable) {
      *result = wrap_optional_once(std::move(*result));
    }

    return result;
  }

  auto ensure_struct(json_value const& schema, std::string type_name, std::string const& path)
    -> std::expected<std::string, std::string> {
    if (active_paths_.contains(path)) {
      return std::unexpected(make_error(path, "recursive schemas are not supported"));
    }

    if (auto const iter = path_to_type_.find(path); iter != path_to_type_.end()) {
      return iter->second;
    }

    auto const* properties_node = find_member(schema, "properties");
    if (properties_node != nullptr && get_object(*properties_node) == nullptr) {
      return std::unexpected(make_error(path, "`properties` must be an object"));
    }

    auto const* required_node = find_member(schema, "required");
    if (required_node != nullptr && get_array(*required_node) == nullptr) {
      return std::unexpected(make_error(path, "`required` must be an array"));
    }

    type_name = reserve_unique_name(used_type_names_, std::move(type_name));
    path_to_type_.emplace(path, type_name);
    active_paths_.insert(path);

    std::set<std::string, std::less<>> required_fields;
    if (auto const* required_entries = required_node != nullptr ? get_array(*required_node) : nullptr;
        required_entries != nullptr) {
      for (auto const& required_entry : *required_entries) {
        auto const* required_name = get_string(required_entry);
        if (required_name == nullptr) {
          active_paths_.erase(path);
          return std::unexpected(make_error(path, "`required` entries must be strings"));
        }
        required_fields.insert(*required_name);
      }
    }

    struct_definition definition{.type_name = type_name, .fields = {}};
    std::unordered_set<std::string> used_member_names;

    if (auto const* properties = properties_node != nullptr ? get_object(*properties_node) : nullptr;
        properties != nullptr) {
      for (auto const& [json_name, property_schema] : *properties) {
        auto member_name = reserve_unique_name(used_member_names, sanitize_member_name(json_name));
        auto property_type =
          resolve_type(property_schema, choose_nested_name(type_name, json_name), path + "/properties/" +
                                                                    encode_json_pointer_token(json_name));
        if (!property_type) {
          active_paths_.erase(path);
          return std::unexpected(std::move(property_type.error()));
        }

        if (!required_fields.contains(json_name)) {
          *property_type = wrap_optional_once(std::move(*property_type));
        }

        definition.fields.push_back(field_definition{
          .json_name = json_name,
          .member_name = std::move(member_name),
          .cpp_type = std::move(*property_type),
        });
      }
    }

    definitions_.push_back(std::move(definition));
    active_paths_.erase(path);
    return type_name;
  }

  auto resolve_ref(std::string_view ref) const -> std::expected<resolved_schema, std::string> {
    if (!ref.starts_with('#')) {
      return std::unexpected(make_error(ref, "only local `$ref` values are supported"));
    }

    if (ref == "#") {
      return resolved_schema{.node = &schema_, .path = "#"};
    }

    if (ref.size() < 2 || ref[1] != '/') {
      return std::unexpected(make_error(ref, "invalid JSON pointer"));
    }

    auto const* current = &schema_;
    std::string current_path = "#";
    std::size_t position = 2;

    while (position <= ref.size()) {
      auto const next_separator = ref.find('/', position);
      auto const token = ref.substr(position, next_separator == std::string_view::npos ? ref.size() - position
                                                                                       : next_separator - position);
      auto const decoded_token = decode_json_pointer_token(token);

      auto const* object = get_object(*current);
      if (object == nullptr) {
        return std::unexpected(make_error(current_path, "JSON pointer walked into a non-object value"));
      }

      auto const iter = object->find(decoded_token);
      if (iter == object->end()) {
        return std::unexpected(make_error(ref, "JSON pointer target was not found"));
      }

      current = &iter->second;
      current_path.append("/");
      current_path.append(encode_json_pointer_token(decoded_token));

      if (next_separator == std::string_view::npos) {
        break;
      }
      position = next_separator + 1;
    }

    return resolved_schema{.node = current, .path = std::move(current_path)};
  }

  auto choose_struct_name(json_value const& schema, std::string_view fallback, std::string_view path) const -> std::string {
    if (path == "#" && !options_.root_name.empty()) {
      return to_pascal_case(options_.root_name);
    }

    if (auto const* title = find_member(schema, "title"); title != nullptr) {
      if (auto const* title_value = get_string(*title); title_value != nullptr && !title_value->empty()) {
        return to_pascal_case(*title_value);
      }
    }

    if (auto const last_token = last_pointer_token(path); !last_token.empty()) {
      return to_pascal_case(last_token);
    }

    if (!fallback.empty()) {
      return to_pascal_case(fallback);
    }

    return "GeneratedObject";
  }

  auto choose_nested_name(std::string_view parent_name, std::string_view child_name) const -> std::string {
    return to_pascal_case(std::string{parent_name} + "_" + std::string{child_name});
  }

  json_value const&                         schema_;
  codegen_options                          options_;
  std::vector<struct_definition>           definitions_;
  std::unordered_map<std::string, std::string> path_to_type_;
  std::unordered_set<std::string>          used_type_names_;
  std::unordered_set<std::string>          active_paths_;
};

}  // namespace internal

inline auto generate_structs_from_json_schema(std::string_view schema_json, codegen_options options = {})
  -> std::expected<std::string, std::string> {
  auto schema = glz::read_json<internal::json_value>(schema_json);
  if (!schema) {
    return std::unexpected("failed to parse schema JSON: " + glz::format_error(schema.error(), schema_json));
  }

  internal::generator generator{*schema, std::move(options)};
  return generator.generate();
}

inline auto generate_structs_from_json_schema_file(std::filesystem::path const& schema_path, codegen_options options = {})
  -> std::expected<std::string, std::string> {
  internal::json_value schema{};
  std::string          buffer;
  auto const           ec = glz::read_file_json(schema, schema_path.string(), buffer);
  if (ec) {
    return std::unexpected("failed to read schema file `" + schema_path.string() + "`: " +
                           glz::format_error(ec, buffer));
  }

  if (options.root_name.empty()) {
    options.root_name = schema_path.stem().string();
  }

  internal::generator generator{schema, std::move(options)};
  return generator.generate();
}

}  // namespace glz_util::schema_codegen

#endif /* __GLZ_UTIL_JSON_SCHEMA_CODEGEN__ */
