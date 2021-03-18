#pragma once

#include <cstdint>
#include <vector>

namespace seqmaker::ais {
using mmsi_t = std::int32_t;
using time_t = std::uint32_t;

struct Point final {
    using value_type = std::int32_t;

    static constexpr value_type MAX_LATITUDE = 180 * 600000;
    static constexpr value_type MIN_LATITUDE = -MAX_LATITUDE;
    static constexpr value_type MAX_LONGITUDE = 180 * 600000;
    static constexpr value_type MIN_LONGITUDE = -MAX_LONGITUDE;

    value_type latitude;    // NOLINT(misc-non-private-member-variables-in-classes)
    value_type longitude;   // NOLINT(misc-non-private-member-variables-in-classes)

    [[nodiscard]] double dist_nm(Point /* other */) const noexcept;

    [[nodiscard]] Point interpolate(Point /* other */, double /* w */) const noexcept;
};

struct Position final {
    time_t t;
    Point x;
};

using Trajectory = std::vector<Position>;

[[nodiscard]] constexpr bool is_valid_mmsi(mmsi_t mmsi) noexcept {
    constexpr mmsi_t MIN_MMSI = 200000000;
    constexpr mmsi_t MAX_MMSI = 799999999;
    return mmsi >= MIN_MMSI and mmsi <= MAX_MMSI;
}

[[nodiscard]] double acc_dist_nm(const std::vector<Point>& /* points */) noexcept;
}   // namespace seqmaker::ais
