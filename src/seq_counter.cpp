#include "seq_counter.hpp"

#include "seq.hpp"

namespace seqmaker {
void SequenceCounter::process(ais::mmsi_t mmsi, const ais::Trajectory& trajectory) noexcept {
    drop_rates_.emplace(mmsi, drop_rate(trajectory, split_args_));
}

[[nodiscard]] std::unordered_map<ais::mmsi_t, double>
SequenceCounter::run(bool apply_low_pass_filter) noexcept {
    Sequencer::run(apply_low_pass_filter);
    return drop_rates_;
}
}   // namespace seqmaker