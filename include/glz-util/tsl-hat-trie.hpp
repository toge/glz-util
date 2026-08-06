#ifndef __GLZ_UTIL_TSL_HAT_TRIE_HPP__
#define __GLZ_UTIL_TSL_HAT_TRIE_HPP__

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "glaze/glaze.hpp"

#if __has_include("tsl/htrie_map.h")
#include "tsl/htrie_map.h"
#endif

#if __has_include("tsl/htrie_set.h")
#include "tsl/htrie_set.h"
#endif

#if __has_include("tsl/htrie_map.h")
template<class CharT,
         class T,
         class Hash = tsl::ah::str_hash<CharT>,
         class KeySizeT = std::uint16_t>
class htrie_map_wrapper : public tsl::htrie_map<CharT, T, Hash, KeySizeT> {
public:
  using key_type = std::basic_string_view<CharT>;
  using base_type = tsl::htrie_map<CharT, T, Hash, KeySizeT>;

  using base_type::base_type;

  /// Glaze の range 検出および反復に使用するイテレータ（ベースクラスに委譲）
  auto begin()       { return base_type::begin(); }
  auto end()         { return base_type::end(); }
  auto begin() const { return base_type::begin(); }
  auto end()   const { return base_type::end(); }

  [[nodiscard]]
  base_type& raw() { return *this; }
  [[nodiscard]]
  const base_type& raw() const { return *this; }
};

#endif

#if __has_include("tsl/htrie_set.h")

template<class CharT,
         class Hash = tsl::ah::str_hash<CharT>,
         class KeySizeT = std::uint16_t>
class htrie_set_wrapper : public tsl::htrie_set<CharT, Hash, KeySizeT> {
public:
  using value_type = std::basic_string_view<CharT>;
  using key_type = value_type;
  using base_type = tsl::htrie_set<CharT, Hash, KeySizeT>;

  using base_type::base_type;

  [[nodiscard]]
  base_type& raw() { return *this; }
  [[nodiscard]]
  const base_type& raw() const { return *this; }
};

#endif

namespace glz {

#if __has_include("tsl/htrie_map.h")

/// htrie_map_wrapper を JSON オブジェクトとして読み書きするためのカスタムシリアライザ
template<class CharT, class T, class Hash, class KeySizeT>
struct meta<htrie_map_wrapper<CharT, T, Hash, KeySizeT>> {
  static constexpr auto custom_read  = true;
  static constexpr auto custom_write = true;
};

/**
 * @brief JSON オブジェクトから htrie_map_wrapper へ読み込む。
 *        一旦 std::map に読み込み、各エントリをトライに挿入する。
 */
template<class CharT, class T, class Hash, class KeySizeT>
struct from<JSON, htrie_map_wrapper<CharT, T, Hash, KeySizeT>> {
  template <auto Opts>
  static auto op(htrie_map_wrapper<CharT, T, Hash, KeySizeT>& value,
                 is_context auto&&                             ctx,
                 auto&&                                        it,
                 auto                                          end) -> void {
    auto entries = std::map<std::basic_string<CharT>, T>{};
    parse<JSON>::op<Opts>(entries, ctx, it, end);
    if (ctx.error != error_code::none) {
      return;
    }

    value.raw().clear();
    for (auto const& [key, val] : entries) {
      value.raw()[std::basic_string_view<CharT>{key}] = val;
    }
  }
};

/**
 * @brief htrie_map_wrapper を JSON オブジェクトとして書き出す。
 *        一旦 std::map に変換してからシリアライズする。
 */
template<class CharT, class T, class Hash, class KeySizeT>
struct to<JSON, htrie_map_wrapper<CharT, T, Hash, KeySizeT>> {
  template <auto Opts>
  static auto op(htrie_map_wrapper<CharT, T, Hash, KeySizeT> const& value,
                 is_context auto&&                                   ctx,
                 auto&&                                              b,
                 auto&                                               ix) -> void {
    auto entries = std::map<std::basic_string<CharT>, T>{};

    // tsl::htrie_map のイテレータは key_buffer()/key_size() でキーを取得する
    for (auto iter = value.raw().begin(); iter != value.raw().end(); ++iter) {
      entries.emplace(std::basic_string<CharT>{iter.key_buffer(), iter.key_size()}, *iter);
    }

    serialize<JSON>::op<Opts>(entries, ctx, b, ix);
  }
};

#endif

#if __has_include("tsl/htrie_set.h")

/// htrie_set_wrapper を JSON 配列として読み書きするためのカスタムシリアライザ
template<class CharT, class Hash, class KeySizeT>
struct meta<htrie_set_wrapper<CharT, Hash, KeySizeT>> {
  static constexpr auto custom_read  = true;
  static constexpr auto custom_write = true;
};

/**
 * @brief JSON 配列から htrie_set_wrapper へ読み込む。
 *        各要素は文字列として解釈され、トライに挿入される。
 */
template<class CharT, class Hash, class KeySizeT>
struct from<JSON, htrie_set_wrapper<CharT, Hash, KeySizeT>> {
  template <auto Opts>
  static auto op(htrie_set_wrapper<CharT, Hash, KeySizeT>& value,
                 is_context auto&&                          ctx,
                 auto&&                                     it,
                 auto                                       end) -> void {
    auto strs = std::vector<std::basic_string<CharT>>{};
    parse<JSON>::op<Opts>(strs, ctx, it, end);
    if (ctx.error != error_code::none) {
      return;
    }

    value.raw().clear();
    for (auto const& s : strs) {
      value.raw().insert(s);
    }
  }
};

/**
 * @brief htrie_set_wrapper を JSON 配列として書き出す。
 *        各キーは文字列として書き出される。
 */
template<class CharT, class Hash, class KeySizeT>
struct to<JSON, htrie_set_wrapper<CharT, Hash, KeySizeT>> {
  template <auto Opts>
  static auto op(htrie_set_wrapper<CharT, Hash, KeySizeT> const& value,
                 is_context auto&&                                ctx,
                 auto&&                                           b,
                 auto&                                            ix) -> void {
    auto keys = std::vector<std::basic_string<CharT>>{};
    keys.reserve(value.raw().size());

    // tsl::htrie_set のイテレータは key_buffer()/key_size() でキーを取得する
    for (auto iter = value.raw().begin(); iter != value.raw().end(); ++iter) {
      keys.emplace_back(iter.key_buffer(), iter.key_size());
    }

    serialize<JSON>::op<Opts>(keys, ctx, b, ix);
  }
};

#endif

} // namespace glz

#endif /* __GLZ_UTIL_TSL_HAT_TRIE_HPP__ */
