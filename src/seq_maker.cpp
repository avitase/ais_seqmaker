#include "seq_maker.hpp"

#include "seq.hpp"

namespace seqmaker {
void SequenceMaker::init(std::size_t n_trajectories) noexcept {
    seqs_.reserve(n_trajectories);
}

void SequenceMaker::process(ais::mmsi_t mmsi, const ais::Trajectory& trajectory) noexcept {
    if (auto stripped_seq = split(trajectory, split_args_); not stripped_seq.empty()) {
        seqs_.emplace(mmsi, stripped_seq);
    }
}

std::unordered_map<ais::mmsi_t, std::vector<ais::Point>>
SequenceMaker::run(bool apply_low_pass_filter) noexcept {
    Sequencer::run(apply_low_pass_filter);
    return seqs_;
}
}   // namespace seqmaker
