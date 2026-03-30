#ifndef __GLZ_UTIL_TSL_HAT_TRIE_HPP__
#define __GLZ_UTIL_TSL_HAT_TRIE_HPP__

#include <cstdint>
#include <map>
#include <string_view>

#if __has_include("tsl/htrie_map.h")
#include "tsl/htrie_map.h"

template<class CharT,
         class T,
         class Hash = tsl::ah::str_hash<CharT>,
         class KeySizeT = std::uint16_t>
class htrie_map_wrapper : public tsl::htrie_map<CharT, T, Hash, KeySizeT> {
public:
  using key_type = std::basic_string_view<CharT>;
  using iterator_t = typename std::map<std::basic_string_view<CharT>, T>::iterator;
  using base_type = tsl::htrie_map<CharT, T, Hash, KeySizeT>;

  using base_type::base_type;

  iterator_t begin() { return iterator_t{}; }
  iterator_t end() { return iterator_t{}; }
  [[nodiscard]]
  base_type& raw() { return *this; }
  [[nodiscard]]
  const base_type& raw() const { return *this; }
};

#endif

#endif /* __GLZ_UTIL_TSL_HAT_TRIE_HPP__ */
