// Copyright 2018 The abel Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <string>

#include <abel/base/profile.h>
#include <abel/chrono/time.h>
#include <benchmark/benchmark.h>

namespace {

//
// Factory functions
//

    void BM_Duration_Factory_Nanoseconds(benchmark::State &state) {
        int64_t i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::nanoseconds(i));
            i += 314159;
        }
    }

    BENCHMARK(BM_Duration_Factory_Nanoseconds);

    void BM_Duration_Factory_Microseconds(benchmark::State &state) {
        int64_t i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::microseconds(i));
            i += 314;
        }
    }

    BENCHMARK(BM_Duration_Factory_Microseconds);

    void BM_Duration_Factory_Milliseconds(benchmark::State &state) {
        int64_t i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::milliseconds(i));
            i += 1;
        }
    }

    BENCHMARK(BM_Duration_Factory_Milliseconds);

    void BM_Duration_Factory_Seconds(benchmark::State &state) {
        int64_t i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::seconds(i));
            i += 1;
        }
    }

    BENCHMARK(BM_Duration_Factory_Seconds);

    void BM_Duration_Factory_Minutes(benchmark::State &state) {
        int64_t i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::minutes(i));
            i += 1;
        }
    }

    BENCHMARK(BM_Duration_Factory_Minutes);

    void BM_Duration_Factory_Hours(benchmark::State &state) {
        int64_t i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::hours(i));
            i += 1;
        }
    }

    BENCHMARK(BM_Duration_Factory_Hours);

    void BM_Duration_Factory_DoubleNanoseconds(benchmark::State &state) {
        double d = 1;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::nanoseconds(d));
            d = d * 1.00000001 + 1;
        }
    }

    BENCHMARK(BM_Duration_Factory_DoubleNanoseconds);

    void BM_Duration_Factory_DoubleMicroseconds(benchmark::State &state) {
        double d = 1e-3;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::microseconds(d));
            d = d * 1.00000001 + 1e-3;
        }
    }

    BENCHMARK(BM_Duration_Factory_DoubleMicroseconds);

    void BM_Duration_Factory_DoubleMilliseconds(benchmark::State &state) {
        double d = 1e-6;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::milliseconds(d));
            d = d * 1.00000001 + 1e-6;
        }
    }

    BENCHMARK(BM_Duration_Factory_DoubleMilliseconds);

    void BM_Duration_Factory_DoubleSeconds(benchmark::State &state) {
        double d = 1e-9;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::seconds(d));
            d = d * 1.00000001 + 1e-9;
        }
    }

    BENCHMARK(BM_Duration_Factory_DoubleSeconds);

    void BM_Duration_Factory_DoubleMinutes(benchmark::State &state) {
        double d = 1e-9;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::minutes(d));
            d = d * 1.00000001 + 1e-9;
        }
    }

    BENCHMARK(BM_Duration_Factory_DoubleMinutes);

    void BM_Duration_Factory_DoubleHours(benchmark::State &state) {
        double d = 1e-9;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::duration::hours(d));
            d = d * 1.00000001 + 1e-9;
        }
    }

    BENCHMARK(BM_Duration_Factory_DoubleHours);

//
// Arithmetic
//

    void BM_Duration_Addition(benchmark::State &state) {
        abel::duration d = abel::duration::nanoseconds(1);
        abel::duration step = abel::duration::milliseconds(1);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d += step);
        }
    }

    BENCHMARK(BM_Duration_Addition);

    void BM_Duration_Subtraction(benchmark::State &state) {
        abel::duration d = abel::duration::seconds(std::numeric_limits<int64_t>::max());
        abel::duration step = abel::duration::milliseconds(1);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d -= step);
        }
    }

    BENCHMARK(BM_Duration_Subtraction);

    void BM_Duration_Multiplication_Fixed(benchmark::State &state) {
        abel::duration d = abel::duration::milliseconds(1);
        abel::duration s;
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(s += d * (i + 1));
            ++i;
        }
    }

    BENCHMARK(BM_Duration_Multiplication_Fixed);

    void BM_Duration_Multiplication_Double(benchmark::State &state) {
        abel::duration d = abel::duration::milliseconds(1);
        abel::duration s;
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(s += d * (i + 1.0));
            ++i;
        }
    }

    BENCHMARK(BM_Duration_Multiplication_Double);

    void BM_Duration_Division_Fixed(benchmark::State &state) {
        abel::duration d = abel::duration::seconds(1);
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d /= i + 1);
            ++i;
        }
    }

    BENCHMARK(BM_Duration_Division_Fixed);

    void BM_Duration_Division_Double(benchmark::State &state) {
        abel::duration d = abel::duration::seconds(1);
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d /= i + 1.0);
            ++i;
        }
    }

    BENCHMARK(BM_Duration_Division_Double);

    void BM_Duration_FDivDuration_Nanoseconds(benchmark::State &state) {
        double d = 1;
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(
                    d += abel::duration::milliseconds(i).float_div_duration(abel::duration::nanoseconds(1)));
            ++i;
        }
    }

    BENCHMARK(BM_Duration_FDivDuration_Nanoseconds);

    void BM_Duration_IDivDuration_Nanoseconds(benchmark::State &state) {
        int64_t a = 1;
        abel::duration ignore;
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(a +=
                                             abel::duration::integer_div_duration(abel::duration::nanoseconds(i),
                                                                        abel::duration::nanoseconds(1), &ignore));
            ++i;
        }
    }

    BENCHMARK(BM_Duration_IDivDuration_Nanoseconds);

    void BM_Duration_IDivDuration_Microseconds(benchmark::State &state) {
        int64_t a = 1;
        abel::duration ignore;
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(a += abel::duration::integer_div_duration(abel::duration::microseconds(i),
                                                                     abel::duration::microseconds(1),
                                                                     &ignore));
            ++i;
        }
    }

    BENCHMARK(BM_Duration_IDivDuration_Microseconds);

    void BM_Duration_IDivDuration_Milliseconds(benchmark::State &state) {
        int64_t a = 1;
        abel::duration ignore;
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(a += abel::duration::integer_div_duration(abel::duration::milliseconds(i),
                                                                     abel::duration::milliseconds(1),
                                                                     &ignore));
            ++i;
        }
    }

    BENCHMARK(BM_Duration_IDivDuration_Milliseconds);

    void BM_Duration_IDivDuration_Seconds(benchmark::State &state) {
        int64_t a = 1;
        abel::duration ignore;
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(
                    a += abel::duration::integer_div_duration(abel::duration::seconds(i), abel::duration::seconds(1),
                                                              &ignore));
            ++i;
        }
    }

    BENCHMARK(BM_Duration_IDivDuration_Seconds);

    void BM_Duration_IDivDuration_Minutes(benchmark::State &state) {
        int64_t a = 1;
        abel::duration ignore;
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(
                    a += abel::duration::integer_div_duration(abel::duration::minutes(i), abel::duration::minutes(1),
                                                              &ignore));
            ++i;
        }
    }

    BENCHMARK(BM_Duration_IDivDuration_Minutes);

    void BM_Duration_IDivDuration_Hours(benchmark::State &state) {
        int64_t a = 1;
        abel::duration ignore;
        int i = 0;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(
                    a += abel::duration::integer_div_duration(abel::duration::hours(i), abel::duration::hours(1),
                                                              &ignore));
            ++i;
        }
    }

    BENCHMARK(BM_Duration_IDivDuration_Hours);

    void BM_Duration_ToInt64Nanoseconds(benchmark::State &state) {
        abel::duration d = abel::duration::seconds(100000);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d.to_int64_nanoseconds());
        }
    }

    BENCHMARK(BM_Duration_ToInt64Nanoseconds);

    void BM_Duration_ToInt64Microseconds(benchmark::State &state) {
        abel::duration d = abel::duration::seconds(100000);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d.to_int64_microseconds());
        }
    }

    BENCHMARK(BM_Duration_ToInt64Microseconds);

    void BM_Duration_ToInt64Milliseconds(benchmark::State &state) {
        abel::duration d = abel::duration::seconds(100000);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d.to_int64_milliseconds());
        }
    }

    BENCHMARK(BM_Duration_ToInt64Milliseconds);

    void BM_Duration_ToInt64Seconds(benchmark::State &state) {
        abel::duration d = abel::duration::seconds(100000);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d.to_int64_seconds());
        }
    }

    BENCHMARK(BM_Duration_ToInt64Seconds);

    void BM_Duration_ToInt64Minutes(benchmark::State &state) {
        abel::duration d = abel::duration::seconds(100000);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d.to_int64_minutes());
        }
    }

    BENCHMARK(BM_Duration_ToInt64Minutes);

    void BM_Duration_ToInt64Hours(benchmark::State &state) {
        abel::duration d = abel::duration::seconds(100000);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d.to_int64_hours());
        }
    }

    BENCHMARK(BM_Duration_ToInt64Hours);

//
// To/FromTimespec
//

    void BM_Duration_ToTimespec_AbelTime(benchmark::State &state) {
        abel::duration d = abel::duration::seconds(1);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d.to_timespec());
        }
    }

    BENCHMARK(BM_Duration_ToTimespec_AbelTime);

    ABEL_NO_INLINE timespec DoubleToTimespec(double seconds) {
        timespec ts;
        ts.tv_sec = seconds;
        ts.tv_nsec = (seconds - ts.tv_sec) * (1000 * 1000 * 1000);
        return ts;
    }

    void BM_Duration_ToTimespec_Double(benchmark::State &state) {
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(DoubleToTimespec(1.0));
        }
    }

    BENCHMARK(BM_Duration_ToTimespec_Double);

    void BM_Duration_FromTimespec_AbelTime(benchmark::State &state) {
        timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        while (state.KeepRunning()) {
            if (++ts.tv_nsec == 1000 * 1000 * 1000) {
                ++ts.tv_sec;
                ts.tv_nsec = 0;
            }
            benchmark::DoNotOptimize(abel::duration::from_timespec(ts));
        }
    }

    BENCHMARK(BM_Duration_FromTimespec_AbelTime);

    ABEL_NO_INLINE double TimespecToDouble(timespec ts) {
        return ts.tv_sec + (ts.tv_nsec / (1000 * 1000 * 1000));
    }

    void BM_Duration_FromTimespec_Double(benchmark::State &state) {
        timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        while (state.KeepRunning()) {
            if (++ts.tv_nsec == 1000 * 1000 * 1000) {
                ++ts.tv_sec;
                ts.tv_nsec = 0;
            }
            benchmark::DoNotOptimize(TimespecToDouble(ts));
        }
    }

    BENCHMARK(BM_Duration_FromTimespec_Double);

//
// String conversions
//

    const char *const kDurations[] = {
            "0",                                   // 0
            "123ns",                               // 1
            "1h2m3s",                              // 2
            "-2h3m4.005006007s",                   // 3
            "2562047788015215h30m7.99999999975s",  // 4
    };
    const int kNumDurations = sizeof(kDurations) / sizeof(kDurations[0]);

    void BM_Duration_FormatDuration(benchmark::State &state) {
        const std::string s = kDurations[state.range(0)];
        state.SetLabel(s);
        abel::duration d;
        abel::parse_duration(kDurations[state.range(0)], &d);
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(d.format_duration());
        }
    }

    BENCHMARK(BM_Duration_FormatDuration)
    ->DenseRange(0, kNumDurations - 1);

    void BM_Duration_ParseDuration(benchmark::State &state) {
        const std::string s = kDurations[state.range(0)];
        state.SetLabel(s);
        abel::duration d;
        while (state.KeepRunning()) {
            benchmark::DoNotOptimize(abel::parse_duration(s, &d));
        }
    }

    BENCHMARK(BM_Duration_ParseDuration)
    ->DenseRange(0, kNumDurations - 1);

}  // namespace
