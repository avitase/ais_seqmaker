#include "seq_diff.hpp"

#include "utility.hpp"

#include <cmath>
#include <utility>

namespace seqmaker {
void SequenceDiff::process(ais::mmsi_t /* mmsi */, const ais::Trajectory& trajectory) noexcept {
    if (trajectory.size() > stride_) {
        std::vector<std::pair<ais::time_t, ais::Point::value_type>> diff;
        diff.reserve(trajectory.size() - stride_);
        utility::adjacent_diff(
            trajectory.begin(),
            trajectory.end(),
            std::back_inserter(diff),
            [](auto pos1, auto pos2) {
                const auto dt = pos2.t - pos1.t;
                const auto dx_nm = pos2.x.dist_nm(pos1.x);

                constexpr double AIS_SCALE = 10000.;
                const auto dx_ais = std::round(dx_nm * AIS_SCALE);

                return std::make_pair(dt,
                                      static_cast<typename decltype(pos1.x)::value_type>(dx_ais));
            },
            stride_);

        diffs_.insert(diffs_.end(), diff.begin(), diff.end());
    }
}

std::vector<std::pair<ais::time_t, ais::Point::value_type>>
SequenceDiff::run(unsigned stride) noexcept {
    stride_ = stride;
    Sequencer::run(false);
    return diffs_;
}
}   // namespace seqmaker
