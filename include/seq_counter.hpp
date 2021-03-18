#pragma once

#include "ais.hpp"
#include "sequencer.hpp"

#include <unordered_map>
#include <utility>

namespace seqmaker {
class SequenceCounter final: public Sequencer {
  private:
    std::unordered_map<ais::mmsi_t, double> drop_rates_;

  public:
    template <typename... Ts>
    explicit SequenceCounter(Ts&&... ts) : Sequencer(std::forward<Ts>(ts)...) {
    }

    ~SequenceCounter() override = default;

    SequenceCounter(const SequenceCounter&) = default;

    SequenceCounter(SequenceCounter&&) = default;

    SequenceCounter& operator=(const SequenceCounter&) = default;

    SequenceCounter& operator=(SequenceCounter&&) = default;

    void init(std::size_t /* n_trajectories */) noexcept override {
    }

    void process(ais::mmsi_t /* mmsi */, const ais::Trajectory& /* trajectory */) noexcept override;

    [[nodiscard]] std::unordered_map<ais::mmsi_t, double>
    run(bool /* apply_low_pass_filter */) noexcept;
};
}   // namespace seqmaker
