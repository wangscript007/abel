//

#include <iostream>
#include <random>

#include <abel/random/random.h>

// This program is used in integration tests.

int main() {
    std::seed_seq seed_seq{1234};
    abel::bit_gen rng(seed_seq);
    constexpr size_t kSequenceLength = 8;
    for (size_t i = 0; i < kSequenceLength; i++) {
        std::cout << rng() << "\n";
    }
    return 0;
}
