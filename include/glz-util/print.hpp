#ifndef __GLZ_UTIL_PRINT__
#define __GLZ_UTIL_PRINT__

#include <iostream>

#include "glaze/glaze.hpp"

namespace glz_util {

template <typename T, std::size_t IDX = 0>
auto print_members(T const& value) {
  if constexpr (IDX < glz::reflect<T>::size) {
    auto const  field_name  = glz::reflect<T>::keys[IDX];
    auto const& field_value = [&]() -> decltype(auto) {
      if constexpr (glz::reflectable<T>) {
        // メタ情報がない場合はstd::tupleにしてから取得する
        return glz::get<IDX>(glz::to_tie(value));
      } else {
        // メタ情報がある場合はメタ情報を利用して取得する
        auto& member = glz::get<IDX>(glz::reflect<T>::values);
        return glz::get_member(value, member);
      }
    }();
    std::cout << IDX << " " << field_name << " " << field_value << '\n';

    print_members<T, IDX + 1>(value);
  }
}

}  // namespace glz_util

#endif /* __GLZ_UTIL_PRINT__ */
