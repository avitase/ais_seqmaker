#include <fmt/format.h>
#include <iterator>
#include <utility>

auto sum_values(const uint8_t* data, size_t size) {
    constexpr auto scale = 1000;

    int value = 0;
    for (std::size_t offset = 0; offset < size; ++offset) {
        value += static_cast<int>(*std::next(data, static_cast<long>(offset))) * scale;
    }
    return value;
}

// Fuzzer that attempts to invoke undefined behavior for signed integer overflow
// cppcheck-suppress unusedFunction symbolName=LLVMFuzzerTestOneInput
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    /*
     * TODO
     */
    sum_values(data, size);
    return 0;
}
