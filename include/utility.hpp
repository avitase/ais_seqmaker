#pragma once

#include "function_traits.hpp"

#include <charconv>
#include <cmath>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace seqmaker::utility {
template <typename TO> [[nodiscard]] TO to(std::string_view from, TO fallback) noexcept {
    std::from_chars(from.data(), from.data() + from.size(), fallback);
    return fallback;
}

namespace detail {
    template <typename F, std::size_t... I>
    [[nodiscard]] auto split_map(std::string_view line,
                                 std::string_view delimiter,
                                 F&& map,
                                 std::index_sequence<I...> /* unused */) {
        constexpr auto N = sizeof...(I);
        std::array<std::string_view, N> tokens;

        const auto* first = line.begin();
        std::size_t i = 0;
        for (; i < N && first != line.end();) {
            const auto* const second = std::find_first_of(first,
                                                          std::cend(line),
                                                          std::cbegin(delimiter),
                                                          std::cend(delimiter));
            if (first != second) {
                tokens[i++]
                    = line.substr(static_cast<std::size_t>(std::distance(line.begin(), first)),
                                  static_cast<std::size_t>(std::distance(first, second)));
            }

            if (second == line.end()) {
                break;
            }

            first = std::next(second);
        }

        if (i != N) {
            throw std::invalid_argument("Invalid data format. Could not find enough columns.");
        }

        return map(tokens[I]...);
    }
}   // namespace detail

template <typename InputIt, typename OutputIt, typename BinaryOp>
void adjacent_diff(InputIt first, InputIt last, OutputIt d_first, BinaryOp op, unsigned stride) {
    if (std::distance(first, last) > stride) {
        auto it = std::next(first, stride);
        do {
            *d_first++ = op(*first++, *it++);
        } while (it != last);
    }
}

template <typename Lambda>
[[nodiscard]] auto split_map(std::string_view line, std::string_view delimiter, Lambda&& map) {
    constexpr auto N = function_traits::lambda_traits<Lambda>::n_args;
    return detail::split_map(line, delimiter, map, std::make_index_sequence<N>{});
}

template <typename T>
[[nodiscard]] inline std::optional<T> time_recorded(std::string_view recv_seconds,
                                                    std::string_view slot_seconds) noexcept {
    auto recv = to<T>(recv_seconds, 0);

    constexpr T slot_max_value = 59;
    auto slot = to<T>(slot_seconds, slot_max_value + 1);

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
    if (recv == 0 || slot > slot_max_value) {
        return std::nullopt;
    }

    constexpr T ONE_MINUTE = 60;
    const auto sec = recv % ONE_MINUTE;
    const auto slot_correction = (sec < slot ? -ONE_MINUTE : 0);
    const auto dt = sec - slot - slot_correction;

    return recv - dt;
}

template <typename InputIt, typename OutputIt, typename BinaryOperation>
[[nodiscard]] auto
low_pass_filter(InputIt first, InputIt last, OutputIt d_first, BinaryOperation binary_op) noexcept {
    if (std::distance(first, last) > 1) {
        auto acc = 1;
        for (; std::next(first) != last; first++) {
            acc = binary_op(*first, *std::next(first)) ? 0 : (acc + 1);
            if (acc < 2) {
                *d_first++ = *first;
            }
        }

        if (acc < 1) {
            *d_first++ = *first;
        }
    }

    return d_first;
}
}   // namespace seqmaker::utility
