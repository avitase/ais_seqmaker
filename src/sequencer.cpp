#include "sequencer.hpp"

#include "io.hpp"
#include "utility.hpp"

#include <cassert>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace seqmaker {
Sequencer::Sequencer(split_args split_args, std::string_view delimiter)
    : delimiter_(delimiter)
    , split_args_(split_args) {
    if (auto read_from_input_stream = not delimiter.empty(); read_from_input_stream) {
        io::process_input_stream([this, delimiter](std::string_view line) {
            auto process_ais_line = [](std::string_view t_str,
                                       std::string_view mmsi_str,
                                       std::string_view slot_str,
                                       std::string_view lat_str,
                                       std::string_view lon_str) {
                auto any_empty = [](auto... x) { return (x.empty() || ...); };
                if (any_empty(t_str, mmsi_str, slot_str, lat_str, lon_str)) {
                    throw std::invalid_argument(
                        "Invalid data format. At least one column is empty.");
                }

                constexpr auto pos_fallback = std::numeric_limits<ais::Point::value_type>::max();

                const auto t = utility::time_recorded<ais::time_t>(t_str, slot_str);
                const auto mmsi = utility::to<ais::mmsi_t>(mmsi_str, 0);
                const auto lat = utility::to<ais::Point::value_type>(lat_str, pos_fallback);
                const auto lon = utility::to<ais::Point::value_type>(lon_str, pos_fallback);

                auto valid_pos = [](auto lat, auto lon) {
                    constexpr auto MAX_LAT = ais::Point::MAX_LATITUDE;
                    constexpr auto MAX_LON = ais::Point::MAX_LONGITUDE;
                    constexpr auto MIN_LAT = ais::Point::MIN_LATITUDE;
                    constexpr auto MIN_LON = ais::Point::MIN_LONGITUDE;
                    static_assert(MAX_LAT < pos_fallback);
                    static_assert(MAX_LON < pos_fallback);
                    return MIN_LAT <= lat and lat <= MAX_LAT and MIN_LON <= lon and lon <= MAX_LON;
                };

                const auto is_valid = ais::is_valid_mmsi(mmsi) and t and valid_pos(lat, lon);
                return is_valid ? std::optional{std::make_pair(
                           mmsi,
                           ais::Position{.t = *t,
                                         .x = ais::Point{.latitude = lat, .longitude = lon}})}
                                : std::nullopt;
            };

            const auto data = utility::split_map(line, delimiter, process_ais_line);
            if (data) {
                const auto [mmsi, trajectory] = *data;
                auto it = this->trajectories_.find(mmsi);
                if (it == this->trajectories_.end()) {
                    it = this->trajectories_.try_emplace(mmsi, 0).first;
                }

                it->second.emplace_back(trajectory);
            }
        });
    }
}

void Sequencer::run(bool apply_low_pass_filter) noexcept {
    init(trajectories_.size());
    for (auto [mmsi, trajectory] : trajectories_) {
        auto by_time = [](auto a, auto b) { return a.t < b.t; };
        std::sort(trajectory.begin(), trajectory.end(), by_time);

        auto time_eq = [](auto a, auto b) { return a.t == b.t; };
        auto last = std::unique(trajectory.begin(), trajectory.end(), time_eq);
        trajectory.erase(last, trajectory.end());

        if (apply_low_pass_filter) {
            auto is_valid = [ds_max = split_args_.ds_max](auto a, auto b) noexcept {
                assert(a.t < b.t);     // NOLINT
                assert(ds_max > 0.);   // NOLINT

                // pieces are already ordered in time
                return a.x.dist_nm(b.x) <= ds_max;
            };
            last = utility::low_pass_filter(trajectory.begin(),
                                            trajectory.end(),
                                            trajectory.begin(),
                                            is_valid),
            trajectory.erase(last, trajectory.end());
        }

        const auto dt = split_args_.seq_length * split_args_.dti;
        if (auto n = trajectory.size(); n > 0 and n * split_args_.dt_max >= dt) {
            auto t_min = trajectory.front().t;
            auto t_max = trajectory.back().t;
            if (t_max - t_min >= dt) {
                process(mmsi, trajectory);
            }
        }
    }
}

void Sequencer::add_trajectory(ais::mmsi_t mmsi, const ais::Trajectory& trajectory) noexcept {
    trajectories_.insert_or_assign(mmsi, trajectory);
}
}   // namespace seqmaker