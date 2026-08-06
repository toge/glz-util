#ifndef __GLZ_UTIL_ENV__
#define __GLZ_UTIL_ENV__

#include <charconv>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include "glaze/glaze.hpp"
#include "glz-util/internal/json_escape.hpp"

namespace glz_util {

namespace internal {

/// @brief Glaze の reflectable/meta 両方に対応したフィールドアクセスヘルパー
template<typename T, std::size_t IDX>
auto get_field(T& result) -> decltype(auto) {
  if constexpr (glz::reflectable<T>) {
    // メタ情報がない場合は std::tuple に変換してから取得する
    return glz::get<IDX>(glz::to_tie(result));
  } else {
    // メタ情報がある場合はメタ情報を利用して取得する
    auto& member = glz::get<IDX>(glz::reflect<T>::values);
    return glz::get_member(result, member);
  }
}

inline auto make_parse_error_message(std::string_view field_name, std::string_view env, std::string_view detail)
  -> std::string {
  std::string message;
  message.reserve(field_name.size() + env.size() + detail.size() + 48);
  message.append("{\"env\":\"");
  append_json_escaped(message, field_name);
  message.append("\",\"value\":\"");
  append_json_escaped(message, env);
  message.append("\",\"detail\":\"");
  append_json_escaped(message, detail);
  message.append("\"}");
  return message;
}

inline auto make_from_chars_error_message(std::string_view field_name, std::string_view env, std::errc ec) -> std::string {
  return make_parse_error_message(field_name, env, std::make_error_code(ec).message());
}

inline auto make_from_chars_trailing_characters_message(std::string_view field_name, std::string_view env)
  -> std::string {
  return make_parse_error_message(field_name, env, "input contains trailing characters");
}

template<typename T, std::size_t IDX>
auto from_env(T& result) -> std::expected<void, std::string> {
  if constexpr (IDX < glz::reflect<T>::size) {
    auto const field_name = glz::reflect<T>::keys[IDX];
    if (auto const env = getenv(field_name.data()); env != nullptr) {
      auto const field_name_sv = std::string_view{field_name.data(), field_name.size()};
      auto const env_sv = std::string_view{env};
      auto& value = get_field<T, IDX>(result);
      // std::from_chars は std::string への変換に対応していないので、std::string の場合は直接代入する
      if constexpr (std::is_convertible_v<decltype(value), std::string>) {
        value = env;
      } else {
        auto const first = env_sv.data();
        auto const last = first + env_sv.size();
        auto const [ptr, ec] = std::from_chars(first, last, value);
        if (ec != std::errc{}) {
          return std::unexpected(make_from_chars_error_message(field_name_sv, env_sv, ec));
        }
        if (ptr != last) {
          return std::unexpected(make_from_chars_trailing_characters_message(field_name_sv, env_sv));
        }
      }
    }
    return from_env<T, IDX + 1>(result);
  }

  return {};
}

template<auto Opts, typename T, std::size_t IDX>
auto from_env(T& result) -> std::expected<void, std::string> {
  if constexpr (IDX < glz::reflect<T>::size) {
    auto const field_name = glz::reflect<T>::keys[IDX];
    if (auto const env = getenv(field_name.data()); env != nullptr) {
      auto const field_name_sv = std::string_view{field_name.data(), field_name.size()};
      auto const env_sv = std::string_view{env};
      auto& value = get_field<T, IDX>(result);
      // value が std::string に変換できる型の場合は、env の値をそのまま代入する
      if constexpr (std::is_convertible_v<decltype(value), std::string>) {
        auto const env_size = env_sv.size();
        if (env_size >= 2 && env[0] == '"' && env[env_size - 1] == '"') {
          if (auto const ec = glz::read<Opts>(value, env_sv); ec) {
            return std::unexpected(make_parse_error_message(field_name_sv, env_sv, glz::format_error(ec, env_sv)));
          }
        } else {
          value = env;
        }
      } else {
        if (auto const ec = glz::read<Opts>(value, env_sv); ec) {
          return std::unexpected(make_parse_error_message(field_name_sv, env_sv, glz::format_error(ec, env_sv)));
        }
      }
    }
    return from_env<Opts, T, IDX + 1>(result);
  }

  return {};
}

}

template<typename T>
auto from_env() -> std::expected<T, std::string> {
  T result{};
  if (auto status = internal::from_env<T, 0>(result); !status) {
    return std::unexpected(std::move(status.error()));
  }
  return result;
}

template<auto Opts, typename T>
auto from_env() -> std::expected<T, std::string> {
  T result{};
  if (auto status = internal::from_env<Opts, T, 0>(result); !status) {
    return std::unexpected(std::move(status.error()));
  }
  return result;
}

}

#endif /* __GLZ_UTIL_ENV__ */
