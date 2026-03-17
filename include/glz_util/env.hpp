#ifndef __GLZ_UTIL_ENV__
#define __GLZ_UTIL_ENV__

#include <stdexcept>

#include "glaze/glaze.hpp"

namespace glz_util {

namespace internal {

template<typename T, std::size_t IDX>
auto from_env(T& result) {
  if constexpr (IDX < glz::reflect<T>::size) {
    auto const field_name = glz::reflect<T>::keys[IDX];
    if (auto const env = getenv(field_name.data()); env != nullptr) {
      auto& value = [&]() -> decltype(auto) {
        if constexpr (glz::reflectable<T>) {
          // メタ情報がない場合はstd::tupleにしてから取得する
          return glz::get<IDX>(glz::to_tie(result));
        } else {
          // メタ情報がある場合はメタ情報を利用して取得する
          auto& member = glz::get<IDX>(glz::reflect<T>::values);
          return glz::get_member(result, member);
        }
      }();
      // std::from_charsはstd::stringへの変換に対応していないので、std::stringの場合は直接代入する
      if constexpr (std::is_convertible_v<decltype(value), std::string>) {
        value = env;
      } else {
        std::from_chars(env, env + std::strlen(env), value);
      }
    }
    from_env<T, IDX + 1>(result);
  }
}

template<auto Opts, typename T, std::size_t IDX>
auto from_env(T& result) {
  if constexpr (IDX < glz::reflect<T>::size) {
    auto const field_name = glz::reflect<T>::keys[IDX];
    if (auto const env = getenv(field_name.data()); env != nullptr) {
      auto& value = [&]() -> decltype(auto) {
          if constexpr (glz::reflectable<T>) {
            return glz::get<IDX>(glz::to_tie(result));
          } else {
            auto& member = glz::get<IDX>(glz::reflect<T>::values);
            return glz::get_member(result, member);
          }
      }();
      // valueがstd::stringに変換できる型の場合は、envの値をそのまま代入する
      if constexpr (std::is_convertible_v<decltype(value), std::string>) {
        if (env[0] == '"' && env[std::strlen(env) - 1] == '"') {
          glz::read<Opts>(value, env);
        } else {
          value = env;
        }
      } else {
          glz::read<Opts>(value, env);
      }
    }
    from_env<Opts, T, IDX + 1>(result);
  }
}

}

template<typename T>
auto from_env() {
  T result{};
  internal::from_env<T, 0>(result);
  return result;
}

template<auto Opts, typename T>
auto from_env() {
  T result{};
  internal::from_env<Opts, T, 0>(result);
  return result;
}

}

#endif /* __GLZ_UTIL_ENV__ */
