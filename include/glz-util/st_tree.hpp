#ifndef __GLZ_UTIL_ST_TREE_HPP__
#define __GLZ_UTIL_ST_TREE_HPP__

#include <concepts>
#include <memory>
#include <type_traits>
#include <vector>

#include "glaze/glaze.hpp"

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#if __has_include("st_tree.h")
#include "st_tree.h"
#endif

#if __has_include("st_tree.h")
template<class Data,
         class StorageModel = st_tree::raw<>,
         class Alloc = std::allocator<Data>>
class st_tree_wrapper : public st_tree::tree<Data, StorageModel, Alloc> {
public:
  using base_type = st_tree::tree<Data, StorageModel, Alloc>;
  using data_type = Data;
  using storage_model_type = StorageModel;

  using base_type::base_type;

  [[nodiscard]]
  auto raw() -> base_type& { return *this; }

  [[nodiscard]]
  auto raw() const -> const base_type& { return *this; }
};
#endif

namespace glz_util::detail {

#if __has_include("st_tree.h")
template<class StorageModel>
struct supports_st_tree_storage : std::false_type {};

template<class Unused>
struct supports_st_tree_storage<st_tree::raw<Unused>> : std::true_type {};

template<class Compare>
struct supports_st_tree_storage<st_tree::ordered<Compare>> : std::true_type {};

template<class StorageModel>
inline constexpr auto supports_st_tree_storage_v =
  supports_st_tree_storage<std::remove_cvref_t<StorageModel>>::value;

template<class Data>
struct st_tree_json_node {
  Data value{};
  std::vector<st_tree_json_node<Data>> children{};
};

template<class T>
concept st_tree_json_wrapper =
  requires(T& value) {
    typename std::remove_cvref_t<T>::base_type;
    typename std::remove_cvref_t<T>::data_type;
    typename std::remove_cvref_t<T>::storage_model_type;
    { value.raw() } -> std::same_as<typename std::remove_cvref_t<T>::base_type&>;
  } &&
  std::derived_from<std::remove_cvref_t<T>, typename std::remove_cvref_t<T>::base_type> &&
  supports_st_tree_storage_v<typename std::remove_cvref_t<T>::storage_model_type>;

template<st_tree_json_wrapper Wrapper>
struct st_tree_json_traits {
  using wrapper_type = std::remove_cvref_t<Wrapper>;
  using tree_type = typename wrapper_type::base_type;
  using node_type = typename tree_type::node_type;
  using data_type = typename wrapper_type::data_type;
  using node_json_type = st_tree_json_node<data_type>;

  static auto read(wrapper_type& value, node_json_type const& data) -> void {
    value.raw().clear();
    value.raw().insert(data.value);
    append_children(value.raw().root(), data.children);
  }

  [[nodiscard]]
  static auto write(wrapper_type const& value) -> node_json_type {
    return write_node(value.raw().root());
  }

private:
  static auto append_children(node_type& parent,
                              std::vector<node_json_type> const& children) -> void {
    for (auto const& child : children) {
      auto const inserted = parent.insert(child.value);
      append_children(*inserted, child.children);
    }
  }

  [[nodiscard]]
  static auto write_node(node_type const& node) -> node_json_type {
    auto children = std::vector<node_json_type>{};
    children.reserve(node.size());
    for (auto const& child : node) {
      children.push_back(write_node(child));
    }
    return node_json_type{node.data(), std::move(children)};
  }
};
#endif

} // namespace glz_util::detail

namespace glz {

#if __has_include("st_tree.h")
template<class Data>
struct meta<glz_util::detail::st_tree_json_node<Data>> {
  using T = glz_util::detail::st_tree_json_node<Data>;
  static constexpr auto value = array(&T::value, &T::children);
};

template<class Data, class StorageModel, class Alloc>
struct meta<st_tree_wrapper<Data, StorageModel, Alloc>> {
  static constexpr auto custom_read = true;
  static constexpr auto custom_write = true;
};

template<class T>
  requires glz_util::detail::st_tree_json_wrapper<T>
struct from<JSON, T> {
  template <auto Opts>
  static auto op(T& value, is_context auto&& ctx, auto&& it, auto end) -> void {
    if (skip_ws<Opts>(ctx, it, end)) {
      return;
    }

    if (it == end) {
      ctx.error = error_code::unexpected_end;
      return;
    }

    if (*it != '[') {
      ctx.error = error_code::syntax_error;
      return;
    }

    auto lookahead = it;
    ++lookahead;

    if (skip_ws<Opts>(ctx, lookahead, end)) {
      return;
    }

    if (lookahead != end && *lookahead == ']') {
      ++lookahead;
      it = lookahead;
      value.raw().clear();
      return;
    }

    using traits = glz_util::detail::st_tree_json_traits<T>;
    auto data = typename traits::node_json_type{};
    parse<JSON>::op<Opts>(data, ctx, it, end);
    if (ctx.error != error_code::none) {
      return;
    }

    traits::read(value, data);
  }
};

template<class T>
  requires glz_util::detail::st_tree_json_wrapper<T>
struct to<JSON, T> {
  template <auto Opts>
  static auto op(T const& value, is_context auto&& ctx, auto&& b, auto& ix) -> void {
    if (value.raw().empty()) {
      auto const empty = std::vector<int>{};
      serialize<JSON>::op<Opts>(empty, ctx, b, ix);
      return;
    }

    using traits = glz_util::detail::st_tree_json_traits<T>;
    auto data = traits::write(value);
    serialize<JSON>::op<Opts>(data, ctx, b, ix);
  }
};
#endif

} // namespace glz

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

#endif /* __GLZ_UTIL_ST_TREE_HPP__ */
