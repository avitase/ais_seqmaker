#pragma once

#include "ais.hpp"
#include "sequencer.hpp"

#include <string_view>
#include <utility>
#include <vector>

namespace seqmaker {
class SequenceDiff final: public Sequencer {
  private:
    unsigned stride_{};
    std::vector<std::pair<ais::time_t, ais::Point::value_type>> diffs_;

  public:
    explicit SequenceDiff(std::string_view delimiter)
        : Sequencer(split_args{.seq_length = 0, .dt_max = 1, .dti = 0, .ds_max = 0., .v_min = 0.},
                    delimiter) {
    }

    ~SequenceDiff() override = default;

    SequenceDiff(const SequenceDiff&) = default;

    SequenceDiff(SequenceDiff&&) = default;

    SequenceDiff& operator=(const SequenceDiff&) = default;

    SequenceDiff& operator=(SequenceDiff&&) = default;

    void init(std::size_t /* n_trajectories */) noexcept override {
    }

    void process(ais::mmsi_t /* mmsi */, const ais::Trajectory& /* trajectory */) noexcept override;

    std::vector<std::pair<ais::time_t, ais::Point::value_type>> run(unsigned /* stride */) noexcept;
};
}   // namespace seqmaker
