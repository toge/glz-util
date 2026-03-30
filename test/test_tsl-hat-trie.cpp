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
    auto const ec = glz::write_json(target, buffer);
    REQUIRE(!ec);
  }

  REQUIRE(buffer == "[\"E\",\"G\",\"A\",\"H\",\"D\",\"B\",\"C\",\"F\"]");

  {
    auto test = htrie_map_wrapper<char, std::string>{};
    REQUIRE(test.raw().empty());
  }

  {
    auto target = htrie_map_wrapper<char, std::string>{};

    auto buffer = R"({"123":"hello","987":"world"})";

    auto const ec = glz::read_json(target, buffer);
    REQUIRE(!ec);

    auto const& target2 = target.raw();

    REQUIRE(target2.at("123") == "hello");
    REQUIRE(target2.at("987") == "world");
  }
#endif
}

TEST_CASE("glaze wrapper for tsl::htrie_set") {
#if __has_include("tsl/htrie_set.h")
  {
    auto target = htrie_set_wrapper<char>{};
    REQUIRE(target.raw().empty());
  }

  {
    auto target = htrie_set_wrapper<char>{};
    auto buffer = R"(["123","987"])";

    auto const ec = glz::read_json(target, buffer);

    REQUIRE(!ec);

    auto const& raw = target.raw();

    REQUIRE(raw.count("123") == 1);
    REQUIRE(raw.count("987") == 1);
  }
#endif
}
