//

#include <array>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>
#include <abel/log/logging.h>
#include <abel/base/profile.h>
#include <abel/container/inlined_vector.h>
#include <abel/strings/str_cat.h>

namespace {

    void BM_inline_vectorFill(benchmark::State &state) {
        const int len = state.range(0);
        abel::inline_vector<int, 8> v;
        v.reserve(len);
        for (auto _ : state) {
            v.resize(0);  // Use resize(0) as inline_vector releases storage on clear().
            for (int i = 0; i < len; ++i) {
                v.push_back(i);
            }
            benchmark::DoNotOptimize(v);
        }
    }

    BENCHMARK(BM_inline_vectorFill)
    ->Range(1, 256);

    void BM_inline_vectorFillRange(benchmark::State &state) {
        const int len = state.range(0);
        const std::vector<int> src(len, len);
        abel::inline_vector<int, 8> v;
        v.reserve(len);
        for (auto _ : state) {
            benchmark::DoNotOptimize(src);
            v.assign(src.begin(), src.end());
            benchmark::DoNotOptimize(v);
        }
    }

    BENCHMARK(BM_inline_vectorFillRange)
    ->Range(1, 256);

    void BM_StdVectorFill(benchmark::State &state) {
        const int len = state.range(0);
        std::vector<int> v;
        v.reserve(len);
        for (auto _ : state) {
            v.clear();
            for (int i = 0; i < len; ++i) {
                v.push_back(i);
            }
            benchmark::DoNotOptimize(v);
        }
    }

    BENCHMARK(BM_StdVectorFill)
    ->Range(1, 256);

// The purpose of the next two benchmarks is to verify that
// abel::inline_vector is efficient when moving is more efficent than
// copying. To do so, we use strings that are larger than the short
// string optimization.
    bool StringRepresentedInline(std::string s) {
        const char *chars = s.data();
        std::string s1 = std::move(s);
        return s1.data() != chars;
    }

    int GetNonShortStringOptimizationSize() {
        for (int i = 24; i <= 192; i *= 2) {
            if (!StringRepresentedInline(std::string(i, 'A'))) {
                return i;
            }
        }
        DLOG_CRITICAL("Failed to find a std::string larger than the short std::string optimization");
        return -1;
    }

    void BM_inline_vectorFillString(benchmark::State &state) {
        const int len = state.range(0);
        const int no_sso = GetNonShortStringOptimizationSize();
        std::string strings[4] = {std::string(no_sso, 'A'), std::string(no_sso, 'B'),
                                  std::string(no_sso, 'C'), std::string(no_sso, 'D')};

        for (auto _ : state) {
            abel::inline_vector<std::string, 8> v;
            for (int i = 0; i < len; i++) {
                v.push_back(strings[i & 3]);
            }
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * len);
    }

    BENCHMARK(BM_inline_vectorFillString)
    ->Range(0, 1024);

    void BM_StdVectorFillString(benchmark::State &state) {
        const int len = state.range(0);
        const int no_sso = GetNonShortStringOptimizationSize();
        std::string strings[4] = {std::string(no_sso, 'A'), std::string(no_sso, 'B'),
                                  std::string(no_sso, 'C'), std::string(no_sso, 'D')};

        for (auto _ : state) {
            std::vector<std::string> v;
            for (int i = 0; i < len; i++) {
                v.push_back(strings[i & 3]);
            }
        }
        state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * len);
    }

    BENCHMARK(BM_StdVectorFillString)
    ->Range(0, 1024);

    struct Buffer {  // some arbitrary structure for benchmarking.
        char *base;
        int length;
        int capacity;
        void *user_data;
    };

    void BM_inline_vectorAssignments(benchmark::State &state) {
        const int len = state.range(0);
        using BufferVec = abel::inline_vector<Buffer, 2>;

        BufferVec src;
        src.resize(len);

        BufferVec dst;
        for (auto _ : state) {
            benchmark::DoNotOptimize(dst);
            benchmark::DoNotOptimize(src);
            dst = src;
        }
    }

    BENCHMARK(BM_inline_vectorAssignments)
    ->Arg(0)
    ->Arg(1)
    ->Arg(2)
    ->Arg(3)
    ->Arg(4)
    ->Arg(20);

    void BM_CreateFromContainer(benchmark::State &state) {
        for (auto _ : state) {
            abel::inline_vector<int, 4> src{1, 2, 3};
            benchmark::DoNotOptimize(src);
            abel::inline_vector<int, 4> dst(std::move(src));
            benchmark::DoNotOptimize(dst);
        }
    }

    BENCHMARK(BM_CreateFromContainer);

    struct LargeCopyableOnly {
        LargeCopyableOnly() : d(1024, 17) {}

        LargeCopyableOnly(const LargeCopyableOnly &o) = default;

        LargeCopyableOnly &operator=(const LargeCopyableOnly &o) = default;

        std::vector<int> d;
    };

    struct LargeCopyableSwappable {
        LargeCopyableSwappable() : d(1024, 17) {}

        LargeCopyableSwappable(const LargeCopyableSwappable &o) = default;

        LargeCopyableSwappable &operator=(LargeCopyableSwappable o) {
            using std::swap;
            swap(*this, o);
            return *this;
        }

        friend void swap(LargeCopyableSwappable &a, LargeCopyableSwappable &b) {
            using std::swap;
            swap(a.d, b.d);
        }

        std::vector<int> d;
    };

    struct LargeCopyableMovable {
        LargeCopyableMovable() : d(1024, 17) {}
        // Use implicitly defined copy and move.

        std::vector<int> d;
    };

    struct LargeCopyableMovableSwappable {
        LargeCopyableMovableSwappable() : d(1024, 17) {}

        LargeCopyableMovableSwappable(const LargeCopyableMovableSwappable &o) =
        default;

        LargeCopyableMovableSwappable(LargeCopyableMovableSwappable &&o) = default;

        LargeCopyableMovableSwappable &operator=(LargeCopyableMovableSwappable o) {
            using std::swap;
            swap(*this, o);
            return *this;
        }

        LargeCopyableMovableSwappable &operator=(LargeCopyableMovableSwappable &&o) =
        default;

        friend void swap(LargeCopyableMovableSwappable &a,
                         LargeCopyableMovableSwappable &b) {
            using std::swap;
            swap(a.d, b.d);
        }

        std::vector<int> d;
    };

    template<typename element_type>
    void BM_SwapElements(benchmark::State &state) {
        const int len = state.range(0);
        using Vec = abel::inline_vector<element_type, 32>;
        Vec a(len);
        Vec b;
        for (auto _ : state) {
            using std::swap;
            benchmark::DoNotOptimize(a);
            benchmark::DoNotOptimize(b);
            swap(a, b);
        }
    }

    BENCHMARK_TEMPLATE(BM_SwapElements, LargeCopyableOnly
    )->Range(0, 1024);
    BENCHMARK_TEMPLATE(BM_SwapElements, LargeCopyableSwappable
    )->Range(0, 1024);
    BENCHMARK_TEMPLATE(BM_SwapElements, LargeCopyableMovable
    )->Range(0, 1024);
    BENCHMARK_TEMPLATE(BM_SwapElements, LargeCopyableMovableSwappable
    )
    ->Range(0, 1024);

// The following benchmark is meant to track the efficiency of the vector size
// as a function of stored type via the benchmark label. It is not meant to
// output useful sizeof operator performance. The loop is a dummy operation
// to fulfill the requirement of running the benchmark.
    template<typename VecType>
    void BM_Sizeof(benchmark::State &state) {
        int size = 0;
        for (auto _ : state) {
            VecType vec;
            size = sizeof(vec);
        }
        state.SetLabel(abel::string_cat("sz=", size));
    }

    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<char, 1>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<char, 4>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<char, 7>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<char, 8>
    );

    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<int, 1>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<int, 4>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<int, 7>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<int, 8>
    );

    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<void *, 1>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<void *, 4>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<void *, 7>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<void *, 8>
    );

    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<std::string, 1>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<std::string, 4>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<std::string, 7>
    );
    BENCHMARK_TEMPLATE(BM_Sizeof, abel::inline_vector<std::string, 8>
    );

    void BM_inline_vectorIndexInlined(benchmark::State &state) {
        abel::inline_vector<int, 8> v = {1, 2, 3, 4, 5, 6, 7};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v[4]);
        }
    }

    BENCHMARK(BM_inline_vectorIndexInlined);

    void BM_inline_vectorIndexExternal(benchmark::State &state) {
        abel::inline_vector<int, 8> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v[4]);
        }
    }

    BENCHMARK(BM_inline_vectorIndexExternal);

    void BM_StdVectorIndex(benchmark::State &state) {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v[4]);
        }
    }

    BENCHMARK(BM_StdVectorIndex);

    void BM_inline_vectorDataInlined(benchmark::State &state) {
        abel::inline_vector<int, 8> v = {1, 2, 3, 4, 5, 6, 7};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v.data());
        }
    }

    BENCHMARK(BM_inline_vectorDataInlined);

    void BM_inline_vectorDataExternal(benchmark::State &state) {
        abel::inline_vector<int, 8> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v.data());
        }
        state.SetItemsProcessed(16 * static_cast<int64_t>(state.iterations()));
    }

    BENCHMARK(BM_inline_vectorDataExternal);

    void BM_StdVectorData(benchmark::State &state) {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v.data());
        }
        state.SetItemsProcessed(16 * static_cast<int64_t>(state.iterations()));
    }

    BENCHMARK(BM_StdVectorData);

    void BM_inline_vectorSizeInlined(benchmark::State &state) {
        abel::inline_vector<int, 8> v = {1, 2, 3, 4, 5, 6, 7};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v.size());
        }
    }

    BENCHMARK(BM_inline_vectorSizeInlined);

    void BM_inline_vectorSizeExternal(benchmark::State &state) {
        abel::inline_vector<int, 8> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v.size());
        }
    }

    BENCHMARK(BM_inline_vectorSizeExternal);

    void BM_StdVectorSize(benchmark::State &state) {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v.size());
        }
    }

    BENCHMARK(BM_StdVectorSize);

    void BM_inline_vectorEmptyInlined(benchmark::State &state) {
        abel::inline_vector<int, 8> v = {1, 2, 3, 4, 5, 6, 7};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v.empty());
        }
    }

    BENCHMARK(BM_inline_vectorEmptyInlined);

    void BM_inline_vectorEmptyExternal(benchmark::State &state) {
        abel::inline_vector<int, 8> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v.empty());
        }
    }

    BENCHMARK(BM_inline_vectorEmptyExternal);

    void BM_StdVectorEmpty(benchmark::State &state) {
        std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        for (auto _ : state) {
            benchmark::DoNotOptimize(v);
            benchmark::DoNotOptimize(v.empty());
        }
    }

    BENCHMARK(BM_StdVectorEmpty);

    constexpr size_t kInlinedCapacity = 4;
    constexpr size_t kLargeSize = kInlinedCapacity * 2;
    constexpr size_t kSmallSize = kInlinedCapacity / 2;
    constexpr size_t kBatchSize = 100;

#define ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_FunctionTemplate, T) \
  BENCHMARK_TEMPLATE(BM_FunctionTemplate, T, kLargeSize);        \
  BENCHMARK_TEMPLATE(BM_FunctionTemplate, T, kSmallSize)

#define ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_FunctionTemplate, T)      \
  BENCHMARK_TEMPLATE(BM_FunctionTemplate, T, kLargeSize, kLargeSize); \
  BENCHMARK_TEMPLATE(BM_FunctionTemplate, T, kLargeSize, kSmallSize); \
  BENCHMARK_TEMPLATE(BM_FunctionTemplate, T, kSmallSize, kLargeSize); \
  BENCHMARK_TEMPLATE(BM_FunctionTemplate, T, kSmallSize, kSmallSize)

    template<typename T>
    using InlVec = abel::inline_vector<T, kInlinedCapacity>;

    struct TrivialType {
        size_t val;
    };

    class NontrivialType {
    public:
        ABEL_NO_INLINE NontrivialType() : val_() {
            benchmark::DoNotOptimize(*this);
        }

        ABEL_NO_INLINE NontrivialType(const NontrivialType &other)
                : val_(other.val_) {
            benchmark::DoNotOptimize(*this);
        }

        ABEL_NO_INLINE NontrivialType &operator=(
                const NontrivialType &other) {
            val_ = other.val_;
            benchmark::DoNotOptimize(*this);
            return *this;
        }

        ABEL_NO_INLINE ~NontrivialType() noexcept {
            benchmark::DoNotOptimize(*this);
        }

    private:
        size_t val_;
    };

    template<typename T, typename PrepareVecFn, typename TestVecFn>
    void BatchedBenchmark(benchmark::State &state, PrepareVecFn prepare_vec,
                          TestVecFn test_vec) {
        std::array<InlVec<T>, kBatchSize> vector_batch{};

        while (state.KeepRunningBatch(kBatchSize)) {
            // Prepare batch
            state.PauseTiming();
            for (size_t i = 0; i < kBatchSize; ++i) {
                prepare_vec(vector_batch.data() + i, i);
            }
            benchmark::DoNotOptimize(vector_batch);
            state.ResumeTiming();

            // Test batch
            for (size_t i = 0; i < kBatchSize; ++i) {
                test_vec(vector_batch.data() + i, i);
            }
        }
    }

    template<typename T, size_t ToSize>
    void BM_ConstructFromSize(benchmark::State &state) {
        using VecT = InlVec<T>;
        auto size = ToSize;
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */ [](InlVec<T> *vec, size_t) { vec->~VecT(); },
                /* test_vec = */
                [&](void *ptr, size_t) {
                    benchmark::DoNotOptimize(size);
                    ::new(ptr) VecT(size);
                });
    }

    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_ConstructFromSize, TrivialType);
    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_ConstructFromSize, NontrivialType);

    template<typename T, size_t ToSize>
    void BM_ConstructFromSizeRef(benchmark::State &state) {
        using VecT = InlVec<T>;
        auto size = ToSize;
        auto ref = T();
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */ [](InlVec<T> *vec, size_t) { vec->~VecT(); },
                /* test_vec = */
                [&](void *ptr, size_t) {
                    benchmark::DoNotOptimize(size);
                    benchmark::DoNotOptimize(ref);
                    ::new(ptr) VecT(size, ref);
                });
    }

    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_ConstructFromSizeRef, TrivialType);
    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_ConstructFromSizeRef, NontrivialType);

    template<typename T, size_t ToSize>
    void BM_ConstructFromRange(benchmark::State &state) {
        using VecT = InlVec<T>;
        std::array<T, ToSize> arr{};
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */ [](InlVec<T> *vec, size_t) { vec->~VecT(); },
                /* test_vec = */
                [&](void *ptr, size_t) {
                    benchmark::DoNotOptimize(arr);
                    ::new(ptr) VecT(arr.begin(), arr.end());
                });
    }

    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_ConstructFromRange, TrivialType);
    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_ConstructFromRange, NontrivialType);

    template<typename T, size_t ToSize>
    void BM_ConstructFromCopy(benchmark::State &state) {
        using VecT = InlVec<T>;
        VecT other_vec(ToSize);
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) { vec->~VecT(); },
                /* test_vec = */
                [&](void *ptr, size_t) {
                    benchmark::DoNotOptimize(other_vec);
                    ::new(ptr) VecT(other_vec);
                });
    }

    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_ConstructFromCopy, TrivialType);
    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_ConstructFromCopy, NontrivialType);

    template<typename T, size_t ToSize>
    void BM_ConstructFromMove(benchmark::State &state) {
        using VecT = InlVec<T>;
        std::array<VecT, kBatchSize> vector_batch{};
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [&](InlVec<T> *vec, size_t i) {
                    vector_batch[i].clear();
                    vector_batch[i].resize(ToSize);
                    vec->~VecT();
                },
                /* test_vec = */
                [&](void *ptr, size_t i) {
                    benchmark::DoNotOptimize(vector_batch[i]);
                    ::new(ptr) VecT(std::move(vector_batch[i]));
                });
    }

    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_ConstructFromMove, TrivialType);
    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_ConstructFromMove, NontrivialType);

    template<typename T, size_t FromSize, size_t ToSize>
    void BM_AssignSizeRef(benchmark::State &state) {
        auto size = ToSize;
        auto ref = T();
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */ [](InlVec<T> *vec, size_t) { vec->resize(FromSize); },
                /* test_vec = */
                [&](InlVec<T> *vec, size_t) {
                    benchmark::DoNotOptimize(size);
                    benchmark::DoNotOptimize(ref);
                    vec->assign(size, ref);
                });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_AssignSizeRef, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_AssignSizeRef, NontrivialType);

    template<typename T, size_t FromSize, size_t ToSize>
    void BM_AssignRange(benchmark::State &state) {
        std::array<T, ToSize> arr{};
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */ [](InlVec<T> *vec, size_t) { vec->resize(FromSize); },
                /* test_vec = */
                [&](InlVec<T> *vec, size_t) {
                    benchmark::DoNotOptimize(arr);
                    vec->assign(arr.begin(), arr.end());
                });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_AssignRange, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_AssignRange, NontrivialType);

    template<typename T, size_t FromSize, size_t ToSize>
    void BM_AssignFromCopy(benchmark::State &state) {
        InlVec<T> other_vec(ToSize);
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */ [](InlVec<T> *vec, size_t) { vec->resize(FromSize); },
                /* test_vec = */
                [&](InlVec<T> *vec, size_t) {
                    benchmark::DoNotOptimize(other_vec);
                    *vec = other_vec;
                });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_AssignFromCopy, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_AssignFromCopy, NontrivialType);

    template<typename T, size_t FromSize, size_t ToSize>
    void BM_AssignFromMove(benchmark::State &state) {
        using VecT = InlVec<T>;
        std::array<VecT, kBatchSize> vector_batch{};
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [&](InlVec<T> *vec, size_t i) {
                    vector_batch[i].clear();
                    vector_batch[i].resize(ToSize);
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [&](InlVec<T> *vec, size_t i) {
                    benchmark::DoNotOptimize(vector_batch[i]);
                    *vec = std::move(vector_batch[i]);
                });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_AssignFromMove, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_AssignFromMove, NontrivialType);

    template<typename T, size_t FromSize, size_t ToSize>
    void BM_ResizeSize(benchmark::State &state) {
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) {
                    vec->clear();
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [](InlVec<T> *vec, size_t) { vec->resize(ToSize); });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_ResizeSize, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_ResizeSize, NontrivialType);

    template<typename T, size_t FromSize, size_t ToSize>
    void BM_ResizeSizeRef(benchmark::State &state) {
        auto t = T();
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) {
                    vec->clear();
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [&](InlVec<T> *vec, size_t) {
                    benchmark::DoNotOptimize(t);
                    vec->resize(ToSize, t);
                });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_ResizeSizeRef, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_ResizeSizeRef, NontrivialType);

    template<typename T, size_t FromSize, size_t ToSize>
    void BM_InsertSizeRef(benchmark::State &state) {
        auto t = T();
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) {
                    vec->clear();
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [&](InlVec<T> *vec, size_t) {
                    benchmark::DoNotOptimize(t);
                    auto *pos = vec->data() + (vec->size() / 2);
                    vec->insert(pos, t);
                });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_InsertSizeRef, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_InsertSizeRef, NontrivialType);

    template<typename T, size_t FromSize, size_t ToSize>
    void BM_InsertRange(benchmark::State &state) {
        InlVec<T> other_vec(ToSize);
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) {
                    vec->clear();
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [&](InlVec<T> *vec, size_t) {
                    benchmark::DoNotOptimize(other_vec);
                    auto *pos = vec->data() + (vec->size() / 2);
                    vec->insert(pos, other_vec.begin(), other_vec.end());
                });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_InsertRange, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_InsertRange, NontrivialType);

    template<typename T, size_t FromSize>
    void BM_EmplaceBack(benchmark::State &state) {
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) {
                    vec->clear();
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [](InlVec<T> *vec, size_t) { vec->emplace_back(); });
    }

    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_EmplaceBack, TrivialType);
    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_EmplaceBack, NontrivialType);

    template<typename T, size_t FromSize>
    void BM_PopBack(benchmark::State &state) {
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) {
                    vec->clear();
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [](InlVec<T> *vec, size_t) { vec->pop_back(); });
    }

    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_PopBack, TrivialType);
    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_PopBack, NontrivialType);

    template<typename T, size_t FromSize>
    void BM_EraseOne(benchmark::State &state) {
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) {
                    vec->clear();
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [](InlVec<T> *vec, size_t) {
                    auto *pos = vec->data() + (vec->size() / 2);
                    vec->erase(pos);
                });
    }

    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_EraseOne, TrivialType);
    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_EraseOne, NontrivialType);

    template<typename T, size_t FromSize>
    void BM_EraseRange(benchmark::State &state) {
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) {
                    vec->clear();
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [](InlVec<T> *vec, size_t) {
                    auto *pos = vec->data() + (vec->size() / 2);
                    vec->erase(pos, pos + 1);
                });
    }

    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_EraseRange, TrivialType);
    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_EraseRange, NontrivialType);

    template<typename T, size_t FromSize>
    void BM_Clear(benchmark::State &state) {
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */ [](InlVec<T> *vec, size_t) { vec->resize(FromSize); },
                /* test_vec = */ [](InlVec<T> *vec, size_t) { vec->clear(); });
    }

    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_Clear, TrivialType);
    ABEL_INTERNAL_BENCHMARK_ONE_SIZE(BM_Clear, NontrivialType);

    template<typename T, size_t FromSize, size_t ToCapacity>
    void BM_Reserve(benchmark::State &state) {
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) {
                    vec->clear();
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [](InlVec<T> *vec, size_t) { vec->reserve(ToCapacity); });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_Reserve, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_Reserve, NontrivialType);

    template<typename T, size_t FromCapacity, size_t ToCapacity>
    void BM_ShrinkToFit(benchmark::State &state) {
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [](InlVec<T> *vec, size_t) {
                    vec->clear();
                    vec->resize(ToCapacity);
                    vec->reserve(FromCapacity);
                },
                /* test_vec = */ [](InlVec<T> *vec, size_t) { vec->shrink_to_fit(); });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_ShrinkToFit, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_ShrinkToFit, NontrivialType);

    template<typename T, size_t FromSize, size_t ToSize>
    void BM_Swap(benchmark::State &state) {
        using VecT = InlVec<T>;
        std::array<VecT, kBatchSize> vector_batch{};
        BatchedBenchmark<T>(
                state,
                /* prepare_vec = */
                [&](InlVec<T> *vec, size_t i) {
                    vector_batch[i].clear();
                    vector_batch[i].resize(ToSize);
                    vec->resize(FromSize);
                },
                /* test_vec = */
                [&](InlVec<T> *vec, size_t i) {
                    using std::swap;
                    benchmark::DoNotOptimize(vector_batch[i]);
                    swap(*vec, vector_batch[i]);
                });
    }

    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_Swap, TrivialType);
    ABEL_INTERNAL_BENCHMARK_TWO_SIZE(BM_Swap, NontrivialType);

}  // namespace
