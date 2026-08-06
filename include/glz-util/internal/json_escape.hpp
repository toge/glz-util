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
      case '\b':
        out.append("\\b");
        break;
      case '\f':
        out.append("\\f");
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
      default: {
        auto const uc = static_cast<unsigned char>(c);
        if (uc < 0x20) {
          // RFC 8259: U+0000〜U+001F の残りの制御文字を \u00XX 形式でエスケープ
          static constexpr auto hex = std::string_view{"0123456789abcdef"};
          out.append("\\u00");
          out.push_back(hex[(uc >> 4) & 0x0F]);
          out.push_back(hex[uc & 0x0F]);
        } else {
          out.push_back(c);
        }
        break;
      }
    }
  }
}

}  // namespace glz_util::internal

#endif /* GLZ_UTIL_INTERNAL_JSON_ESCAPE_HPP */
