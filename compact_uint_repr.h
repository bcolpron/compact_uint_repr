#include <concepts>
#include <istream>
#include <iterator>
#include <numeric>

namespace compact_uint_repr {

template <std::unsigned_integral T>
inline auto constexpr max_repr_bytes = sizeof(T) * 8 / 7 + 1;

template<std::unsigned_integral T>
void serialize(T v, std::ostream& out) {
  T p = T{1} << ((max_repr_bytes<T> - 1) * 7);

  while (p != 1u) {
    if (const auto c = v / p; c != 0) {
      out.put(static_cast<char>(0x80 + int8_t(c)));
      v -= c * p;
    }
    p >>= 7;
  }
  out.put(static_cast<char>(v));
}

template <std::unsigned_integral T, std::ranges::range R>
T deserialize(const R& in) {
  T v = 0;
  T c = 0;
  for (auto i = std::begin(in); i != std::end(in);) {
    c = *i++;
    v += (c & 0x7F);
    if (c & 0x80) {
      if (v > std::numeric_limits<T>::max() >> 7)
        throw std::invalid_argument("too much data for requested type");
      v <<= 7;
    } else if (i != std::end(in)) {
      throw std::invalid_argument("extra data bytes");
    }
  }
  if (v == 0u && std::begin(in) == std::end(in))
    throw std::invalid_argument("empty input");
  else if (c & 0x80)
    throw std::invalid_argument("unfinished sequence");
  return v;
}

}  // namespace compact_uint_repr