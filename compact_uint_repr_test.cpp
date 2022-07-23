#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "compact_uint_repr.h"

#include <doctest/doctest.h>

using namespace compact_uint_repr;

template <std::unsigned_integral T>
auto serialize(T v) {
  std::ostringstream buf;
  serialize(v, buf);
  const auto s = buf.str();
  return std::vector<uint8_t>(begin(s), end(s));
}

TEST_CASE("Compact Int") {
  SUBCASE("simple serialization") {
    const auto s = serialize(42u);
    CHECK(s.size() == 1);
    CHECK(s[0] == 0x2a);
  }

  SUBCASE("seralization 128") {
    const auto s = serialize(128u);
    CHECK(s.size() == 2);
    CHECK(s[0] == 0x81);
    CHECK(s[1] == 0x00);
  }

  SUBCASE("seralization 170") { CHECK(serialize(170u)[1] == 0x2a); }

  SUBCASE("seralization 693546") {
    const auto s = serialize(693546u);
    CHECK(s.size() == 3);
    CHECK(s[0] == 0xaa);
    CHECK(s[1] == 0xaa);
    CHECK(s[2] == 0x2a);
  }

  SUBCASE("smallest type") {
    const auto s = serialize(255u);
    CHECK(s.size() == 2);
    CHECK(s[0] == 0x81);
    CHECK(s[1] == 0x7F);
  }

  SUBCASE("max int16 value") {
    REQUIRE(std::numeric_limits<std::uint16_t>::max() == 0xFFFF);
    const auto s = serialize(std::numeric_limits<std::uint16_t>::max());
    CHECK(s.size() == 3);
    CHECK(s[0] == 0x83);
    CHECK(s[1] == 0xFF);
    CHECK(s[2] == 0x7F);
  }

  SUBCASE("largest uint64 value") {
    const auto s = serialize(std::numeric_limits<std::uint64_t>::max());
    CHECK(s.size() == 10);
    CHECK(s[0] == 0x81);
    CHECK(s[2] == 0xFF);
    CHECK(s[3] == 0xFF);
    CHECK(s[4] == 0xFF);
    CHECK(s[5] == 0xFF);
    CHECK(s[6] == 0xFF);
    CHECK(s[7] == 0xFF);
    CHECK(s[8] == 0xFF);
    CHECK(s[9] == 0x7F);
  }

  SUBCASE("get_max_repr_bytes") {
    CHECK(max_repr_bytes<uint8_t> == 2);
    CHECK(max_repr_bytes<uint16_t> == 3);
    CHECK(max_repr_bytes<uint32_t> == 5);
    CHECK(max_repr_bytes<uint64_t> == 10);
  }
}

TEST_CASE("deserialization") {
  SUBCASE("zéer0") {
    std::vector<uint8_t> buf = {0u};
    CHECK(deserialize<unsigned int>(buf) == 0u);
  }
  SUBCASE("one byte") {
    std::vector<uint8_t> buf = {0x2a};
    CHECK(deserialize<unsigned int>(buf) == 42u);
  }
  SUBCASE("two bytes") {
    std::vector<uint8_t> buf = {0x81, 0x2a};
    CHECK(deserialize<unsigned int>(buf) == 170u);
  }
  SUBCASE("255") {
    std::vector<uint8_t> buf = serialize(255u);
    CHECK(deserialize<std::uint8_t>(buf) == 255u);
  }
  SUBCASE("invalid cases") {
    SUBCASE("empty input") {
      std::vector<uint8_t> buf = {};
      CHECK_THROWS(deserialize<unsigned int>(buf));
    }
    SUBCASE("unfinished sequence") {
      std::vector<uint8_t> buf = {0x80};
      CHECK_THROWS(deserialize<unsigned int>(buf));
    }
    SUBCASE("extra data") {
      std::vector<uint8_t> buf = {0x01, 0x01};
      CHECK_THROWS(deserialize<unsigned int>(buf));
    }
    SUBCASE("too much data for type") {
      std::vector<uint8_t> buf = {0x82, 0x01};
      CHECK_THROWS(deserialize<std::uint8_t>(buf));
    }
  }
}