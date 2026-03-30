#include "catch2/catch_all.hpp"

#include "glaze/glaze.hpp"
#include "glz-util/zfp.hpp"

#include <vector>

TEST_CASE("glaze wrapper for zfp::array1 reads and writes nested json") {
#if __has_include("zfp/array.hpp")
  auto target = zfp_array1_wrapper<double>{16.0};
  auto const input = R"([1.25,2.5,3.75,5.0])";

  auto const read_ec = glz::read_json(target, input);
  REQUIRE(!read_ec);

  REQUIRE(target.raw().size() == 4);
  REQUIRE(target.raw().rate() == Catch::Approx(16.0));

  auto output = std::string{};
  auto const write_ec = glz::write_json(target, output);
  REQUIRE(!write_ec);

  auto values = std::vector<double>{};
  auto const parse_ec = glz::read_json(values, output);
  REQUIRE(!parse_ec);

  REQUIRE(values.size() == 4);
  REQUIRE(values[0] == Catch::Approx(1.25));
  REQUIRE(values[1] == Catch::Approx(2.5));
  REQUIRE(values[2] == Catch::Approx(3.75));
  REQUIRE(values[3] == Catch::Approx(5.0));
#endif
}

TEST_CASE("glaze wrapper for zfp::array2 reads and writes nested json") {
#if __has_include("zfp/array.hpp")
  auto target = zfp_array2_wrapper<double>{16.0};
  auto const input = R"([[1.0,2.0],[3.0,4.0]])";

  auto const read_ec = glz::read_json(target, input);
  REQUIRE(!read_ec);

  REQUIRE(target.raw().size() == 4);
  REQUIRE(target.raw().rate() == Catch::Approx(16.0));

  auto output = std::string{};
  auto const write_ec = glz::write_json(target, output);
  REQUIRE(!write_ec);

  auto values = std::vector<std::vector<double>>{};
  auto const parse_ec = glz::read_json(values, output);
  REQUIRE(!parse_ec);

  REQUIRE(values.size() == 2);
  REQUIRE(values[0].size() == 2);
  REQUIRE(values[1].size() == 2);
  REQUIRE(values[0][0] == Catch::Approx(1.0));
  REQUIRE(values[0][1] == Catch::Approx(2.0));
  REQUIRE(values[1][0] == Catch::Approx(3.0));
  REQUIRE(values[1][1] == Catch::Approx(4.0));
#endif
}

TEST_CASE("glaze wrapper for zfp::array2 rejects ragged json arrays") {
#if __has_include("zfp/array.hpp")
  auto target = zfp_array2_wrapper<double>{16.0};
  auto const input = R"([[1.0],[2.0,3.0]])";

  auto const read_ec = glz::read_json(target, input);

  REQUIRE(read_ec);
#endif
}

TEST_CASE("glaze wrapper for zfp::array3 reads and writes nested json") {
#if __has_include("zfp/array.hpp")
  auto target = zfp_array3_wrapper<double>{16.0};
  auto const input = R"([[[1.0,2.0],[3.0,4.0]],[[5.0,6.0],[7.0,8.0]]])";

  auto const read_ec = glz::read_json(target, input);
  REQUIRE(!read_ec);

  REQUIRE(target.raw().size() == 8);
  REQUIRE(target.raw().rate() == Catch::Approx(16.0));

  auto output = std::string{};
  auto const write_ec = glz::write_json(target, output);
  REQUIRE(!write_ec);

  auto values = std::vector<std::vector<std::vector<double>>>{};
  auto const parse_ec = glz::read_json(values, output);
  REQUIRE(!parse_ec);

  REQUIRE(values.size() == 2);
  REQUIRE(values[0].size() == 2);
  REQUIRE(values[0][0].size() == 2);
  REQUIRE(values[1][1][1] == Catch::Approx(8.0));
#endif
}

TEST_CASE("glaze wrapper for zfp::array4 reads and writes nested json") {
#if __has_include("zfp/array.hpp")
  auto target = zfp_array4_wrapper<double>{16.0};
  auto const input = R"([[[[1.0,2.0]],[[3.0,4.0]]],[[[5.0,6.0]],[[7.0,8.0]]]])";

  auto const read_ec = glz::read_json(target, input);
  REQUIRE(!read_ec);

  REQUIRE(target.raw().size() == 8);
  REQUIRE(target.raw().rate() == Catch::Approx(16.0));

  auto output = std::string{};
  auto const write_ec = glz::write_json(target, output);
  REQUIRE(!write_ec);

  auto values = std::vector<std::vector<std::vector<std::vector<double>>>>{};
  auto const parse_ec = glz::read_json(values, output);
  REQUIRE(!parse_ec);

  REQUIRE(values.size() == 2);
  REQUIRE(values[0].size() == 2);
  REQUIRE(values[0][0].size() == 1);
  REQUIRE(values[0][0][0].size() == 2);
  REQUIRE(values[1][1][0][1] == Catch::Approx(8.0));
#endif
}
