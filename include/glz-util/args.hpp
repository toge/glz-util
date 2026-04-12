#ifndef __GLZ_UTIL_ARGS__
#define __GLZ_UTIL_ARGS__

#include <array>
#include <charconv>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#include "glaze/glaze.hpp"

namespace glz_util {

namespace args_internal {

inline auto append_json_escaped(std::string& out, std::string_view input) -> void {
  for (auto const c : input) {
    switch (c) {
      case '\\':
        out.append("\\\\");
        break;
      case '"':
        out.append("\\\"");
        break;
      case '\n':
        out.append("\\n");
        break;
      case '\r':
        out.append("\\r");
        break;
      case '\t':
        out.append("\\t");
        break;
      default:
        out.push_back(c);
        break;
    }
  }
}

inline auto make_parse_error_message(std::string_view field_name, std::string_view value, std::string_view detail)
  -> std::string {
  auto message = std::string{};
  message.reserve(field_name.size() + value.size() + detail.size() + 48);
  message.append("{\"key\":\"");
  append_json_escaped(message, field_name);
  message.append("\",\"value\":\"");
  append_json_escaped(message, value);
  message.append("\",\"detail\":\"");
  append_json_escaped(message, detail);
  message.append("\"}");
  return message;
}

inline auto make_from_chars_error_message(std::string_view field_name, std::string_view value, std::errc ec)
  -> std::string {
  return make_parse_error_message(field_name, value, std::make_error_code(ec).message());
}

inline auto make_from_chars_trailing_characters_message(std::string_view field_name, std::string_view value)
  -> std::string {
  return make_parse_error_message(field_name, value, "input contains trailing characters");
}

inline auto ascii_lower(char c) -> char {
  if (c >= 'A' && c <= 'Z') {
    return static_cast<char>(c - 'A' + 'a');
  }
  return c;
}

inline auto equals_case_insensitive(std::string_view lhs, std::string_view rhs) -> bool {
  if (lhs.size() != rhs.size()) {
    return false;
  }

  for (auto const i : std::views::iota(std::size_t{0}, lhs.size())) {
    if (ascii_lower(lhs[i]) != ascii_lower(rhs[i])) {
      return false;
    }
  }

  return true;
}

inline auto parse_option(std::string_view arg)
  -> std::optional<std::pair<std::string_view, std::optional<std::string_view>>> {
  auto option = std::string_view{};
  if (arg.starts_with("--")) {
    option = arg.substr(2);
  } else if (arg.starts_with('-')) {
    option = arg.substr(1);
  } else {
    return std::nullopt;
  }

  if (option.empty()) {
    return std::nullopt;
  }

  if (auto const pos = option.find('='); pos != std::string_view::npos) {
    return std::pair{option.substr(0, pos), std::optional{option.substr(pos + 1)}};
  }

  return std::pair{option, std::optional<std::string_view>{}};
}

template <typename T, std::size_t IDX = 0>
auto find_field_index(std::string_view key) -> std::optional<std::size_t> {
  if constexpr (IDX < glz::reflect<T>::size) {
    auto const field_name = glz::reflect<T>::keys[IDX];
    auto const field_name_sv = std::string_view{field_name.data(), field_name.size()};
    if (equals_case_insensitive(field_name_sv, key)) {
      return IDX;
    }
    return find_field_index<T, IDX + 1>(key);
  }

  return std::nullopt;
}

template <typename T>
auto is_known_option(std::string_view arg) -> bool {
  if (auto const option = parse_option(arg)) {
    return find_field_index<T>(option->first).has_value();
  }

  return false;
}

template <typename T>
auto collect_values(int argc, char const* const* argv)
  -> std::array<std::optional<std::string_view>, glz::reflect<T>::size> {
  auto values = std::array<std::optional<std::string_view>, glz::reflect<T>::size>{};

  if (argc <= 1 || argv == nullptr) {
    return values;
  }

  for (auto i = 1; i < argc; ++i) {
    if (argv[i] == nullptr) {
      continue;
    }

    auto const current = std::string_view{argv[i]};
    auto const option = parse_option(current);
    if (!option) {
      continue;
    }

    auto const field_index = find_field_index<T>(option->first);
    if (!field_index) {
      continue;
    }

    if (option->second) {
      values[*field_index] = *option->second;
      continue;
    }

    if (i + 1 >= argc || argv[i + 1] == nullptr) {
      continue;
    }

    auto const next = std::string_view{argv[i + 1]};
    if (is_known_option<T>(next)) {
      continue;
    }

    values[*field_index] = next;
    ++i;
  }

  return values;
}

template <typename T, std::size_t IDX>
auto get_field(T& result) -> decltype(auto) {
  if constexpr (glz::reflectable<T>) {
    return glz::get<IDX>(glz::to_tie(result));
  } else {
    auto& member = glz::get<IDX>(glz::reflect<T>::values);
    return glz::get_member(result, member);
  }
}

template <typename T>
concept from_chars_readable = requires(char const* first, char const* last, T& value) {
  std::from_chars(first, last, value);
};

template <typename Value>
auto parse_plain_value(std::string_view field_name, std::string_view raw, Value& value)
  -> std::expected<void, std::string> {
  if constexpr (std::is_convertible_v<decltype(value), std::string>) {
    value = raw;
    return {};
  } else if constexpr (from_chars_readable<std::remove_cvref_t<Value>>) {
    auto const first = raw.data();
    auto const last = first + raw.size();
    auto const [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{}) {
      return std::unexpected(make_from_chars_error_message(field_name, raw, ec));
    }
    if (ptr != last) {
      return std::unexpected(make_from_chars_trailing_characters_message(field_name, raw));
    }
    return {};
  } else {
    if (auto const ec = glz::read<glz::opts{}>(value, raw); ec) {
      return std::unexpected(make_parse_error_message(field_name, raw, glz::format_error(ec, raw)));
    }
    return {};
  }
}

template <glz::opts Opts, typename Value>
auto parse_opts_value(std::string_view field_name, std::string_view raw, Value& value)
  -> std::expected<void, std::string> {
  if constexpr (std::is_convertible_v<decltype(value), std::string>) {
    if (raw.size() >= 2 && raw.front() == '"' && raw.back() == '"') {
      if (auto const ec = glz::read<Opts>(value, raw); ec) {
        return std::unexpected(make_parse_error_message(field_name, raw, glz::format_error(ec, raw)));
      }
    } else {
      value = raw;
    }
  } else {
    if (auto const ec = glz::read<Opts>(value, raw); ec) {
      return std::unexpected(make_parse_error_message(field_name, raw, glz::format_error(ec, raw)));
    }
  }

  return {};
}

template <typename T, std::size_t IDX>
auto apply_values(T& result, std::array<std::optional<std::string_view>, glz::reflect<T>::size> const& values)
  -> std::expected<void, std::string> {
  if constexpr (IDX < glz::reflect<T>::size) {
    if (values[IDX]) {
      auto const field_name = glz::reflect<T>::keys[IDX];
      auto const field_name_sv = std::string_view{field_name.data(), field_name.size()};
      auto& value = get_field<T, IDX>(result);
      if (auto status = parse_plain_value(field_name_sv, *values[IDX], value); !status) {
        return std::unexpected(std::move(status.error()));
      }
    }

    return apply_values<T, IDX + 1>(result, values);
  }

  return {};
}

template <glz::opts Opts, typename T, std::size_t IDX>
auto apply_values(T& result, std::array<std::optional<std::string_view>, glz::reflect<T>::size> const& values)
  -> std::expected<void, std::string> {
  if constexpr (IDX < glz::reflect<T>::size) {
    if (values[IDX]) {
      auto const field_name = glz::reflect<T>::keys[IDX];
      auto const field_name_sv = std::string_view{field_name.data(), field_name.size()};
      auto& value = get_field<T, IDX>(result);
      if (auto status = parse_opts_value<Opts>(field_name_sv, *values[IDX], value); !status) {
        return std::unexpected(std::move(status.error()));
      }
    }

    return apply_values<Opts, T, IDX + 1>(result, values);
  }

  return {};
}

}

/**
 * @brief Parse a reflected Glaze object from argc/argv.
 * @param argc Argument count received by main.
 * @param argv Argument vector received by main.
 * @return Parsed object on success, or a JSON error payload string on the first parse failure.
 */
template <typename T>
auto from_args(int argc, char const* const* argv) -> std::expected<T, std::string> {
  auto result = T{};
  auto const values = args_internal::collect_values<T>(argc, argv);
  if (auto status = args_internal::apply_values<T, 0>(result, values); !status) {
    return std::unexpected(std::move(status.error()));
  }
  return result;
}

/**
 * @brief Parse a reflected Glaze object from argc/argv using explicit Glaze options.
 * @param argc Argument count received by main.
 * @param argv Argument vector received by main.
 * @return Parsed object on success, or a JSON error payload string on the first parse failure.
 */
template <glz::opts Opts, typename T>
auto from_args(int argc, char const* const* argv) -> std::expected<T, std::string> {
  auto result = T{};
  auto const values = args_internal::collect_values<T>(argc, argv);
  if (auto status = args_internal::apply_values<Opts, T, 0>(result, values); !status) {
    return std::unexpected(std::move(status.error()));
  }
  return result;
}

}

#endif /* __GLZ_UTIL_ARGS__ */
