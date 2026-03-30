#ifndef __GLZ_UTIL_ZFP_HPP__
#define __GLZ_UTIL_ZFP_HPP__

#include <concepts>
#include <cstddef>
#include <vector>

#include "glaze/glaze.hpp"

#if __has_include("zfp/array1.hpp")
#include "zfp/array1.hpp"
#endif

#if __has_include("zfp/array2.hpp")
#include "zfp/array2.hpp"
#endif

#if __has_include("zfp/array3.hpp")
#include "zfp/array3.hpp"
#endif

#if __has_include("zfp/array4.hpp")
#include "zfp/array4.hpp"
#endif

#if __has_include("zfp/array1.hpp")
template<std::floating_point Scalar,
         class Codec = zfp::codec::zfp1<Scalar>,
         class Index = zfp::index::implicit>
class zfp_array1_wrapper : public zfp::array1<Scalar, Codec, Index> {
public:
  using base_type = zfp::array1<Scalar, Codec, Index>;
  using scalar_type = Scalar;

  explicit zfp_array1_wrapper(double rate)
      : configured_rate_(rate) {}

  [[nodiscard]]
  auto configured_rate() const -> double { return configured_rate_; }
  auto set_configured_rate(double rate) -> void { configured_rate_ = rate; }
  [[nodiscard]]
  auto raw() -> base_type& { return *this; }
  [[nodiscard]]
  auto raw() const -> const base_type& { return *this; }

private:
  double configured_rate_{};
};
#endif

#if __has_include("zfp/array2.hpp")
template<std::floating_point Scalar,
         class Codec = zfp::codec::zfp2<Scalar>,
         class Index = zfp::index::implicit>
class zfp_array2_wrapper : public zfp::array2<Scalar, Codec, Index> {
public:
  using base_type = zfp::array2<Scalar, Codec, Index>;
  using scalar_type = Scalar;

  explicit zfp_array2_wrapper(double rate)
      : configured_rate_(rate) {}

  [[nodiscard]]
  auto configured_rate() const -> double { return configured_rate_; }
  auto set_configured_rate(double rate) -> void { configured_rate_ = rate; }
  [[nodiscard]]
  auto raw() -> base_type& { return *this; }
  [[nodiscard]]
  auto raw() const -> const base_type& { return *this; }

private:
  double configured_rate_{};
};
#endif

#if __has_include("zfp/array3.hpp")
template<std::floating_point Scalar,
         class Codec = zfp::codec::zfp3<Scalar>,
         class Index = zfp::index::implicit>
class zfp_array3_wrapper : public zfp::array3<Scalar, Codec, Index> {
public:
  using base_type = zfp::array3<Scalar, Codec, Index>;
  using scalar_type = Scalar;

  explicit zfp_array3_wrapper(double rate)
      : configured_rate_(rate) {}

  [[nodiscard]]
  auto configured_rate() const -> double { return configured_rate_; }
  auto set_configured_rate(double rate) -> void { configured_rate_ = rate; }
  [[nodiscard]]
  auto raw() -> base_type& { return *this; }
  [[nodiscard]]
  auto raw() const -> const base_type& { return *this; }

private:
  double configured_rate_{};
};
#endif

#if __has_include("zfp/array4.hpp")
template<std::floating_point Scalar,
         class Codec = zfp::codec::zfp4<Scalar>,
         class Index = zfp::index::implicit>
class zfp_array4_wrapper : public zfp::array4<Scalar, Codec, Index> {
public:
  using base_type = zfp::array4<Scalar, Codec, Index>;
  using scalar_type = Scalar;

  explicit zfp_array4_wrapper(double rate)
      : configured_rate_(rate) {}

  [[nodiscard]]
  auto configured_rate() const -> double { return configured_rate_; }
  auto set_configured_rate(double rate) -> void { configured_rate_ = rate; }
  [[nodiscard]]
  auto raw() -> base_type& { return *this; }
  [[nodiscard]]
  auto raw() const -> const base_type& { return *this; }

private:
  double configured_rate_{};
};
#endif

namespace glz_util::detail {

template<class T>
concept zfp_json_wrapper = requires(T& value) {
  typename std::remove_cvref_t<T>::scalar_type;
  typename std::remove_cvref_t<T>::base_type;
  { value.configured_rate() } -> std::convertible_to<double>;
  { value.raw() };
};

template<zfp_json_wrapper Wrapper>
struct zfp_json_traits;

#if __has_include("zfp/array1.hpp")
template<std::floating_point Scalar, class Codec, class Index>
struct zfp_json_traits<zfp_array1_wrapper<Scalar, Codec, Index>> {
  using wrapper_type = zfp_array1_wrapper<Scalar, Codec, Index>;
  using scalar_type = Scalar;
  using nested_vector_type = std::vector<scalar_type>;

  static auto read(wrapper_type& value, nested_vector_type const& data) -> void {
    auto next = typename wrapper_type::base_type{data.size(), value.configured_rate()};
    next.set(data.empty() ? nullptr : data.data());
    value.raw() = next;
    value.set_configured_rate(value.raw().rate());
  }

  [[nodiscard]]
  static auto write(wrapper_type const& value) -> nested_vector_type {
    auto data = nested_vector_type(value.raw().size());
    value.raw().get(data.empty() ? nullptr : data.data());
    return data;
  }
};
#endif

#if __has_include("zfp/array2.hpp")
template<std::floating_point Scalar, class Codec, class Index>
struct zfp_json_traits<zfp_array2_wrapper<Scalar, Codec, Index>> {
  using wrapper_type = zfp_array2_wrapper<Scalar, Codec, Index>;
  using scalar_type = Scalar;
  using nested_vector_type = std::vector<std::vector<scalar_type>>;

  static auto read(wrapper_type& value,
                   nested_vector_type const& data,
                   glz::is_context auto&& ctx) -> void {
    if (data.empty()) {
      value.raw() = typename wrapper_type::base_type{0, 0, value.configured_rate()};
      value.set_configured_rate(value.raw().rate());
      return;
    }

    auto const width = data.front().size();
    for (auto const& row : data) {
      if (row.size() != width) {
        ctx.error = glz::error_code::constraint_violated;
        ctx.custom_error_message = "zfp_array2_wrapper requires rectangular nested arrays";
        return;
      }
    }

    auto flat = std::vector<scalar_type>{};
    flat.reserve(data.size() * width);
    for (auto const& row : data) {
      flat.insert(flat.end(), row.begin(), row.end());
    }

    auto next = typename wrapper_type::base_type{width, data.size(), value.configured_rate()};
    next.set(flat.empty() ? nullptr : flat.data());
    value.raw() = next;
    value.set_configured_rate(value.raw().rate());
  }

  [[nodiscard]]
  static auto write(wrapper_type const& value) -> nested_vector_type {
    auto flat = std::vector<scalar_type>(value.raw().size());
    value.raw().get(flat.empty() ? nullptr : flat.data());

    auto data = nested_vector_type(value.raw().size_y(),
                                   std::vector<scalar_type>(value.raw().size_x()));
    auto index = std::size_t{};
    for (auto& row : data) {
      for (auto& element : row) {
        element = flat[index++];
      }
    }
    return data;
  }
};
#endif

#if __has_include("zfp/array3.hpp")
template<std::floating_point Scalar, class Codec, class Index>
struct zfp_json_traits<zfp_array3_wrapper<Scalar, Codec, Index>> {
  using wrapper_type = zfp_array3_wrapper<Scalar, Codec, Index>;
  using scalar_type = Scalar;
  using nested_vector_type = std::vector<std::vector<std::vector<scalar_type>>>;

  static auto read(wrapper_type& value,
                   nested_vector_type const& data,
                   glz::is_context auto&& ctx) -> void {
    if (data.empty()) {
      value.raw() = typename wrapper_type::base_type{0, 0, 0, value.configured_rate()};
      value.set_configured_rate(value.raw().rate());
      return;
    }

    auto const height = data.front().size();
    auto const width = height == 0 ? std::size_t{} : data.front().front().size();

    for (auto const& plane : data) {
      if (plane.size() != height) {
        ctx.error = glz::error_code::constraint_violated;
        ctx.custom_error_message = "zfp_array3_wrapper requires rectangular nested arrays";
        return;
      }
      for (auto const& row : plane) {
        if (row.size() != width) {
          ctx.error = glz::error_code::constraint_violated;
          ctx.custom_error_message = "zfp_array3_wrapper requires rectangular nested arrays";
          return;
        }
      }
    }

    auto flat = std::vector<scalar_type>{};
    flat.reserve(data.size() * height * width);
    for (auto const& plane : data) {
      for (auto const& row : plane) {
        flat.insert(flat.end(), row.begin(), row.end());
      }
    }

    auto next = typename wrapper_type::base_type{width, height, data.size(), value.configured_rate()};
    next.set(flat.empty() ? nullptr : flat.data());
    value.raw() = next;
    value.set_configured_rate(value.raw().rate());
  }

  [[nodiscard]]
  static auto write(wrapper_type const& value) -> nested_vector_type {
    auto flat = std::vector<scalar_type>(value.raw().size());
    value.raw().get(flat.empty() ? nullptr : flat.data());

    auto data = nested_vector_type(
      value.raw().size_z(),
      std::vector<std::vector<scalar_type>>(
        value.raw().size_y(),
        std::vector<scalar_type>(value.raw().size_x())));
    auto index = std::size_t{};
    for (auto& plane : data) {
      for (auto& row : plane) {
        for (auto& element : row) {
          element = flat[index++];
        }
      }
    }
    return data;
  }
};
#endif

#if __has_include("zfp/array4.hpp")
template<std::floating_point Scalar, class Codec, class Index>
struct zfp_json_traits<zfp_array4_wrapper<Scalar, Codec, Index>> {
  using wrapper_type = zfp_array4_wrapper<Scalar, Codec, Index>;
  using scalar_type = Scalar;
  using nested_vector_type = std::vector<std::vector<std::vector<std::vector<scalar_type>>>>;

  static auto read(wrapper_type& value,
                   nested_vector_type const& data,
                   glz::is_context auto&& ctx) -> void {
    if (data.empty()) {
      value.raw() = typename wrapper_type::base_type{0, 0, 0, 0, value.configured_rate()};
      value.set_configured_rate(value.raw().rate());
      return;
    }

    auto const depth = data.front().size();
    auto const height = depth == 0 ? std::size_t{} : data.front().front().size();
    auto const width =
      (depth == 0 || height == 0) ? std::size_t{} : data.front().front().front().size();

    for (auto const& hyperplane : data) {
      if (hyperplane.size() != depth) {
        ctx.error = glz::error_code::constraint_violated;
        ctx.custom_error_message = "zfp_array4_wrapper requires rectangular nested arrays";
        return;
      }
      for (auto const& plane : hyperplane) {
        if (plane.size() != height) {
          ctx.error = glz::error_code::constraint_violated;
          ctx.custom_error_message = "zfp_array4_wrapper requires rectangular nested arrays";
          return;
        }
        for (auto const& row : plane) {
          if (row.size() != width) {
            ctx.error = glz::error_code::constraint_violated;
            ctx.custom_error_message = "zfp_array4_wrapper requires rectangular nested arrays";
            return;
          }
        }
      }
    }

    auto flat = std::vector<scalar_type>{};
    flat.reserve(data.size() * depth * height * width);
    for (auto const& hyperplane : data) {
      for (auto const& plane : hyperplane) {
        for (auto const& row : plane) {
          flat.insert(flat.end(), row.begin(), row.end());
        }
      }
    }

    auto next = typename wrapper_type::base_type{
      width, height, depth, data.size(), value.configured_rate()};
    next.set(flat.empty() ? nullptr : flat.data());
    value.raw() = next;
    value.set_configured_rate(value.raw().rate());
  }

  [[nodiscard]]
  static auto write(wrapper_type const& value) -> nested_vector_type {
    auto flat = std::vector<scalar_type>(value.raw().size());
    value.raw().get(flat.empty() ? nullptr : flat.data());

    auto data = nested_vector_type(
      value.raw().size_w(),
      std::vector<std::vector<std::vector<scalar_type>>>(
        value.raw().size_z(),
        std::vector<std::vector<scalar_type>>(
          value.raw().size_y(),
          std::vector<scalar_type>(value.raw().size_x()))));
    auto index = std::size_t{};
    for (auto& hyperplane : data) {
      for (auto& plane : hyperplane) {
        for (auto& row : plane) {
          for (auto& element : row) {
            element = flat[index++];
          }
        }
      }
    }
    return data;
  }
};
#endif

} // namespace glz_util::detail

namespace glz {

#if __has_include("zfp/array1.hpp")
template<std::floating_point Scalar, class Codec, class Index>
struct meta<zfp_array1_wrapper<Scalar, Codec, Index>> {
  static constexpr auto custom_read = true;
  static constexpr auto custom_write = true;
};
#endif

#if __has_include("zfp/array2.hpp")
template<std::floating_point Scalar, class Codec, class Index>
struct meta<zfp_array2_wrapper<Scalar, Codec, Index>> {
  static constexpr auto custom_read = true;
  static constexpr auto custom_write = true;
};
#endif

#if __has_include("zfp/array3.hpp")
template<std::floating_point Scalar, class Codec, class Index>
struct meta<zfp_array3_wrapper<Scalar, Codec, Index>> {
  static constexpr auto custom_read = true;
  static constexpr auto custom_write = true;
};
#endif

#if __has_include("zfp/array4.hpp")
template<std::floating_point Scalar, class Codec, class Index>
struct meta<zfp_array4_wrapper<Scalar, Codec, Index>> {
  static constexpr auto custom_read = true;
  static constexpr auto custom_write = true;
};
#endif

template<class T>
  requires glz_util::detail::zfp_json_wrapper<T>
struct from<JSON, T> {
  template <auto Opts>
  static auto op(T& value, is_context auto&& ctx, auto&& it, auto end) -> void {
    using traits = glz_util::detail::zfp_json_traits<T>;
    auto data = typename traits::nested_vector_type{};
    parse<JSON>::op<Opts>(data, ctx, it, end);
    if (ctx.error != error_code::none) {
      return;
    }

    if constexpr (requires { traits::read(value, data, ctx); }) {
      traits::read(value, data, ctx);
    }
    else {
      traits::read(value, data);
    }
  }
};

template<class T>
  requires glz_util::detail::zfp_json_wrapper<T>
struct to<JSON, T> {
  template <auto Opts>
  static auto op(T const& value, is_context auto&& ctx, auto&& b, auto& ix) -> void {
    using traits = glz_util::detail::zfp_json_traits<T>;
    auto data = traits::write(value);
    serialize<JSON>::op<Opts>(data, ctx, b, ix);
  }
};

} // namespace glz

#endif /* __GLZ_UTIL_ZFP_HPP__ */
