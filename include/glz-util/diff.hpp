#ifndef __GLZ_UTIL_DIFF__
#define __GLZ_UTIL_DIFF__

#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "glaze/glaze.hpp"

namespace glz_util {

/**
 * @brief Represents one changed reflected field between two objects.
 */
struct FieldDiff {
  std::string key;    ///< glz::meta で定義されたキー名
  std::string before; ///< glz::write_json で文字列化した旧値
  std::string after;  ///< glz::write_json で文字列化した新値
};

namespace diff_internal {

template <typename T>
concept reflected_object = glz::reflectable<T> || glz::glaze_object_t<T>;

template <typename Field>
concept json_writable = requires(Field const& field) {
  glz::write_json(field);
};

template <std::size_t IDX, typename T>
auto get_field(T&& value) -> decltype(auto) {
  using U = std::remove_cvref_t<T>;

  if constexpr (glz::reflectable<U>) {
    return glz::get<IDX>(glz::to_tie(std::forward<T>(value)));
  } else {
    auto& member = glz::get<IDX>(glz::reflect<U>::values);
    return glz::get_member(std::forward<T>(value), member);
  }
}

template <typename T, std::size_t IDX>
using field_type_t = std::remove_cvref_t<decltype(get_field<IDX>(std::declval<T const&>()))>;

template <typename T, std::size_t IDX = 0>
consteval auto all_fields_json_writable() -> bool {
  if constexpr (IDX < glz::reflect<T>::size) {
    return json_writable<field_type_t<T, IDX>> && all_fields_json_writable<T, IDX + 1>();
  } else {
    return true;
  }
}

template <typename Field>
auto write_field_json(std::string_view key, Field const& field) -> std::string {
  auto json = glz::write_json(field);
  if (!json) {
    throw std::runtime_error("glz_util::diff_members failed to serialize field: " + std::string{key});
  }
  return std::move(*json);
}

template <typename T>
auto serialize_fields(T const& value) -> std::vector<std::string> {
  auto result = std::vector<std::string>{};
  result.reserve(glz::reflect<T>::size);

  auto index = std::size_t{0};
  glz::for_each_field(value, [&](auto const& field) {
    auto const key = glz::reflect<T>::keys[index];
    auto const key_sv = std::string_view{key.data(), key.size()};
    result.push_back(write_field_json(key_sv, field));
    ++index;
  });

  return result;
}

}

/**
 * @brief Compare two reflected objects of the same type and return changed fields.
 * @tparam T A Glaze-reflectable type whose fields are writable with glz::write_json.
 * @param before Source object before the change.
 * @param after Source object after the change.
 * @return A list of changed fields in reflected order. Returns an empty vector when there is no difference.
 */
template <typename T>
  requires (diff_internal::reflected_object<T> && diff_internal::all_fields_json_writable<T>())
[[nodiscard]] auto diff_members(T const& before, T const& after) -> std::vector<FieldDiff> {
  auto const after_fields = diff_internal::serialize_fields(after);
  auto diffs = std::vector<FieldDiff>{};

  auto index = std::size_t{0};
  glz::for_each_field(before, [&](auto const& before_field) {
    auto const key = glz::reflect<T>::keys[index];
    auto const key_sv = std::string_view{key.data(), key.size()};
    auto const before_json = diff_internal::write_field_json(key_sv, before_field);
    auto const& after_json = after_fields[index];

    if (before_json != after_json) {
      diffs.push_back(FieldDiff{
        .key = std::string{key_sv},
        .before = before_json,
        .after = after_json,
      });
    }

    ++index;
  });

  return diffs;
}

}  // namespace glz_util

template <>
struct glz::meta<glz_util::FieldDiff> {
  using T = glz_util::FieldDiff;

  static constexpr auto value = glz::object(
    "key", &T::key,
    "before", &T::before,
    "after", &T::after
  );
};

#endif /* __GLZ_UTIL_DIFF__ */
