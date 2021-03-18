#pragma once

#include "ais.hpp"

#include <vector>

namespace seqmaker {
[[nodiscard]] std::vector<ais::Point> interpolate(const ais::Trajectory& /* trajectory */,
                                                  unsigned /* n_grid_points */,
                                                  unsigned /* dt */) noexcept;

struct split_args {
    unsigned seq_length;   // NOLINT
    unsigned dt_max;       // NOLINT
    unsigned dti;          // NOLINT
    double ds_max;         // NOLINT
    double v_min;          // NOLINT
};

[[nodiscard]] std::vector<ais::Point> split(const ais::Trajectory& /* trajectory */,
                                            const split_args& /* args */) noexcept;

[[nodiscard]] double drop_rate(const ais::Trajectory& /* trajectory */,
                               split_args /* split_args */) noexcept;
}   // namespace seqmaker
