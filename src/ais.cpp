#include "ais.hpp"

#include "utility.hpp"

#include <cmath>
#include <numeric>

namespace seqmaker::ais {

[[nodiscard]] double Point::dist_nm(Point other) const noexcept {
    auto ais2deg = [](auto ais) {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        return static_cast<double>(ais) / 600000.;
    };
    auto deg2rad = [](auto deg) {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        return deg / 180. * 3.14159265358979323846;
    };
    auto deg2nm = [](auto deg) {
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        return deg * 60.;
    };

    const auto dlat_deg = ais2deg(latitude - other.latitude);
    const auto dlon_deg = ais2deg(longitude - other.longitude);
    const auto mlat_deg = ais2deg(latitude + other.latitude) / 2.;

    const auto cos_mlat = std::cos(deg2rad(mlat_deg));
    const auto dist_deg
        = std::sqrt(dlat_deg * dlat_deg + cos_mlat * cos_mlat * dlon_deg * dlon_deg);

    return deg2nm(dist_deg);
}

[[nodiscard]] Point Point::interpolate(Point other, double w) const noexcept {
    auto intrplt = [w](value_type a, value_type b) {
        using T = decltype(w);
        return static_cast<value_type>((1. - w) * static_cast<T>(a) + w * static_cast<T>(b));
    };

    return Point{.latitude = intrplt(latitude, other.latitude),
                 .longitude = intrplt(longitude, other.longitude)};
}

[[nodiscard]] double acc_dist_nm(const std::vector<Point>& points) noexcept {
    if (auto n = points.size(); n > 1) {
        std::vector<double> d;
        d.reserve(n - 1);
        // cannot use std::adjacent_difference due to different types ais::Point <-> double
        utility::adjacent_diff(
            points.begin(),
            points.end(),
            std::back_inserter(d),
            [](auto a, auto b) { return a.dist_nm(b); },
            1);
        return std::reduce(d.begin(), d.end(), 0.);
    }

    return 0.;
}
}   // namespace seqmaker::ais
