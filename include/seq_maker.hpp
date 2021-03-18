#pragma once

#include "ais.hpp"
#include "sequencer.hpp"

#include <unordered_map>
#include <utility>
#include <vector>

namespace seqmaker {
class SequenceMaker final: public Sequencer {
  private:
    std::unordered_map<ais::mmsi_t, std::vector<ais::Point>> seqs_;

  public:
    template <typename... Ts>
    explicit SequenceMaker(Ts&&... ts) : Sequencer(std::forward<Ts>(ts)...) {   // NOLINT
    }

    ~SequenceMaker() override = default;

    SequenceMaker(const SequenceMaker&) = default;

    SequenceMaker(SequenceMaker&&) = default;

    SequenceMaker& operator=(const SequenceMaker&) = default;

    SequenceMaker& operator=(SequenceMaker&&) = default;

    void init(std::size_t /* n_trajectories */) noexcept override;

    void process(ais::mmsi_t /* mmsi */, const ais::Trajectory& /* trajectory */) noexcept override;

    std::unordered_map<ais::mmsi_t, std::vector<ais::Point>>
    run(bool /* apply_low_pass_filter */) noexcept;
};
}   // namespace seqmaker