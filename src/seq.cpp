#include "seq.hpp"

namespace seqmaker {
[[nodiscard]] std::vector<ais::Point>
interpolate(const ais::Trajectory& trajectory, unsigned n_grid_points, unsigned dt) noexcept {
    std::vector<ais::Point> seq;
    seq.reserve(n_grid_points);

    std::size_t j = 0U;
    const auto t0 = trajectory.front().t;
    for (auto i = 0U; i < n_grid_points; i++) {
        auto ti = t0 + i * dt;

        while (trajectory[j + 1].t < ti) {
            j += 1;
        }

        auto tj1 = trajectory[j].t;
        auto tj2 = trajectory[j + 1].t;

        auto w = static_cast<double>(ti - tj1) / static_cast<double>(tj2 - tj1);
        seq.emplace_back(trajectory[j].x.interpolate(trajectory[j + 1].x, w));
    }

    return seq;
}

[[nodiscard]] std::vector<ais::Point> split(const ais::Trajectory& trajectory,
                                            const split_args& args) noexcept {
    std::vector<ais::Point> seqs;
    ais::Trajectory buffer;

    seqs.reserve(trajectory.size());
    buffer.reserve(trajectory.size());

    ais::time_t t0 = 0;
    for (auto pos : trajectory) {
        buffer.emplace_back(pos);

        if (buffer.size() == 1) {
            t0 = pos.t;
        } else if (auto last_pos = buffer[buffer.size() - 2];
                   pos.t - last_pos.t > args.dt_max or pos.x.dist_nm(last_pos.x) > args.ds_max) {
            buffer.clear();
            buffer.emplace_back(pos);
            t0 = pos.t;
        } else if (pos.t - t0 >= args.seq_length * args.dti) {
            // (seq_length + 1) grid points for sequence length (seq_length * dti)
            auto seq = interpolate(buffer, args.seq_length + 1, args.dti);

            constexpr auto nm_per_s = 1. / 3600.;
            if (auto d_min = args.v_min * nm_per_s * args.seq_length * args.dti;
                ais::acc_dist_nm(seq) >= d_min) {
                seqs.insert(seqs.end(), seq.begin(), seq.end());
            }
            buffer.clear();
        }
    }

    return seqs;
}

[[nodiscard]] double drop_rate(const ais::Trajectory& trajectory, split_args args) noexcept {
    unsigned total = 0;
    unsigned i = 0;

    ais::Position last_pos{};

    ais::time_t t0 = 0;
    for (auto pos : trajectory) {
        i += 1;

        if (i == 1) {
            t0 = pos.t;
        } else if (pos.t - last_pos.t > args.dt_max or pos.x.dist_nm(last_pos.x) > args.ds_max) {
            i = 1;
            t0 = pos.t;
        } else if (pos.t - t0 >= args.seq_length * args.dti) {
            total += i;
            i = 0;
        }

        last_pos = pos;
    }

    return 1. - static_cast<double>(total) / static_cast<double>(trajectory.size());
}
}   // namespace seqmaker
