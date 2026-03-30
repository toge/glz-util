#include "catch2/catch_all.hpp"

#include "glaze/glaze.hpp"

#if __has_include("tsl/htrie_map.h")
#include "tsl/htrie_map.h"
#include "glz-util/tsl-hat-trie.hpp"
#endif

TEST_CASE("glaze wrapper for tss::httrie_map") {
#if __has_include("tsl/htrie_map.h")
  std::string buffer{};
  {
    auto target = tsl::htrie_map<char, std::string>{};
    target["1"] = "A";
    target["2"] = "B";
    target["3"] = "C";
    target["4"] = "D";
    target["5"] = "E";
    target["6"] = "F";
    target["7"] = "G";
    target["8"] = "H";
    glz::write_json(target, buffer);
  }

  REQUIRE(buffer == "[\"E\",\"G\",\"A\",\"H\",\"D\",\"B\",\"C\",\"F\"]");

  {
    htrie_map_wrapper<char, std::string> test();
  }

  {
    auto target = htrie_map_wrapper<char, std::string>{};

    auto buffer = R"({"123":"hello","987":"world"})";

    glz::read_json(target, buffer);

    auto const& target2 = target.raw();

    REQUIRE(target2.at("123") == "hello");
    REQUIRE(target2.at("987") == "world");
  }
#endif
}
