#pragma once

#include <cstdio>
#include <string_view>
#include <vector>

namespace seqmaker::io {
template <typename F> void process_input_stream(F&& f) {
    constexpr auto BUFFER_SIZE = 1024U;
    std::vector<char> buffer;
    buffer.reserve(BUFFER_SIZE);

    auto i = 0U;
    for (int c = EOF; (c = std::getchar()) != EOF;) {
        if (c == '\n') {
            f(std::string_view(&buffer[0], i));
            buffer.clear();
            i = 0;
        } else {
            buffer.emplace_back(static_cast<char>(c));
            i += 1;
        }
    }
}
}   // namespace seqmaker::io
