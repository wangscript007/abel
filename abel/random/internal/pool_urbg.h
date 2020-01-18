//

#ifndef ABEL_RANDOM_INTERNAL_POOL_URBG_H_
#define ABEL_RANDOM_INTERNAL_POOL_URBG_H_

#include <cinttypes>
#include <limits>

#include <abel/random/internal/traits.h>
#include <abel/types/span.h>

namespace abel {
ABEL_NAMESPACE_BEGIN
namespace random_internal {

// RandenPool is a thread-safe random number generator [random.req.urbg] that
// uses an underlying pool of Randen generators to generate values.  Each thread
// has affinity to one instance of the underlying pool generators.  Concurrent
// access is guarded by a spin-lock.
template <typename T>
class RandenPool {
 public:
  using result_type = T;
  static_assert(std::is_unsigned<result_type>::value,
                "RandenPool template argument must be a built-in unsigned "
                "integer type");

  static constexpr result_type(min)() {
    return (std::numeric_limits<result_type>::min)();
  }

  static constexpr result_type(max)() {
    return (std::numeric_limits<result_type>::max)();
  }

  RandenPool() {}

  // Returns a single value.
    ABEL_FORCE_INLINE result_type operator()() { return Generate(); }

  // Fill data with random values.
  static void Fill(abel::Span<result_type> data);

 protected:
  // Generate returns a single value.
  static result_type Generate();
};

extern template class RandenPool<uint8_t>;
extern template class RandenPool<uint16_t>;
extern template class RandenPool<uint32_t>;
extern template class RandenPool<uint64_t>;

// PoolURBG uses an underlying pool of random generators to implement a
// thread-compatible [random.req.urbg] interface with an internal cache of
// values.
template <typename T, size_t kBufferSize>
class PoolURBG {
  // Inheritance to access the protected static members of RandenPool.
  using unsigned_type = typename make_unsigned_bits<T>::type;
  using PoolType = RandenPool<unsigned_type>;
  using SpanType = abel::Span<unsigned_type>;

  static constexpr size_t kInitialBuffer = kBufferSize + 1;
  static constexpr size_t kHalfBuffer = kBufferSize / 2;

 public:
  using result_type = T;

  static_assert(std::is_unsigned<result_type>::value,
                "PoolURBG must be parameterized by an unsigned integer type");

  static_assert(kBufferSize > 1,
                "PoolURBG must be parameterized by a buffer-size > 1");

  static_assert(kBufferSize <= 256,
                "PoolURBG must be parameterized by a buffer-size <= 256");

  static constexpr result_type(min)() {
    return (std::numeric_limits<result_type>::min)();
  }

  static constexpr result_type(max)() {
    return (std::numeric_limits<result_type>::max)();
  }

  PoolURBG() : next_(kInitialBuffer) {}

  // copy-constructor does not copy cache.
  PoolURBG(const PoolURBG&) : next_(kInitialBuffer) {}
  const PoolURBG& operator=(const PoolURBG&) {
    next_ = kInitialBuffer;
    return *this;
  }

  // move-constructor does move cache.
  PoolURBG(PoolURBG&&) = default;
  PoolURBG& operator=(PoolURBG&&) = default;

    ABEL_FORCE_INLINE result_type operator()() {
    if (next_ >= kBufferSize) {
      next_ = (kBufferSize > 2 && next_ > kBufferSize) ? kHalfBuffer : 0;
      PoolType::Fill(SpanType(reinterpret_cast<unsigned_type*>(state_ + next_),
                              kBufferSize - next_));
    }
    return state_[next_++];
  }

 private:
  // Buffer size.
  size_t next_;  // index within state_
  result_type state_[kBufferSize];
};

}  // namespace random_internal
ABEL_NAMESPACE_END
}  // namespace abel

#endif  // ABEL_RANDOM_INTERNAL_POOL_URBG_H_
