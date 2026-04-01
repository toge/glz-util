#include "catch2/catch_all.hpp"

#include "glaze/glaze.hpp"

#if __has_include("st_tree.h")
#include "glz-util/st_tree.hpp"
#endif

TEST_CASE("glaze wrapper for st_tree raw reads and writes nested json") {
#if __has_include("st_tree.h")
  auto target = st_tree_wrapper<std::string>{};
  auto const input = R"(["root",[["left",[]],["right",[["leaf",[]]]]]])";

  auto const read_ec = glz::read_json(target, input);
  REQUIRE(!read_ec);

  auto const& raw = target.raw();
  REQUIRE(!raw.empty());
  REQUIRE(raw.root().data() == "root");
  REQUIRE(raw.root().size() == 2);
  REQUIRE(raw.root()[0].data() == "left");
  REQUIRE(raw.root()[1].data() == "right");
  REQUIRE(raw.root()[1][0].data() == "leaf");

  auto output = std::string{};
  auto const write_ec = glz::write_json(target, output);
  REQUIRE(!write_ec);
  REQUIRE(output == input);
#endif
}

TEST_CASE("glaze wrapper for st_tree raw maps empty tree to empty json array") {
#if __has_include("st_tree.h")
  auto target = st_tree_wrapper<std::string>{};
  auto const input = std::string{"[]"};

  auto const read_ec = glz::read_json(target, input);
  REQUIRE(!read_ec);
  REQUIRE(target.raw().empty());

  auto output = std::string{};
  auto const write_ec = glz::write_json(target, output);
  REQUIRE(!write_ec);
  REQUIRE(output == input);
#endif
}

TEST_CASE("glaze wrapper for st_tree ordered sorts children during round trip") {
#if __has_include("st_tree.h")
  auto target = st_tree_wrapper<std::string, st_tree::ordered<>>{};
  auto const input = R"(["root",[["c",[]],["a",[]],["b",[]],["b",[]]]])";

  auto const read_ec = glz::read_json(target, input);
  REQUIRE(!read_ec);

  auto const& raw = target.raw();
  REQUIRE(raw.root().size() == 4);

  auto it = raw.root().begin();
  REQUIRE(it->data() == "a");
  ++it;
  REQUIRE(it->data() == "b");
  ++it;
  REQUIRE(it->data() == "b");
  ++it;
  REQUIRE(it->data() == "c");

  auto output = std::string{};
  auto const write_ec = glz::write_json(target, output);
  REQUIRE(!write_ec);
  REQUIRE(output == R"(["root",[["a",[]],["b",[]],["b",[]],["c",[]]]])");
#endif
}
