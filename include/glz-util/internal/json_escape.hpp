#ifndef GLZ_UTIL_INTERNAL_JSON_ESCAPE_HPP
#define GLZ_UTIL_INTERNAL_JSON_ESCAPE_HPP

#include <string>
#include <string_view>

namespace glz_util::internal {

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

}  // namespace glz_util::internal

#endif /* GLZ_UTIL_INTERNAL_JSON_ESCAPE_HPP */
