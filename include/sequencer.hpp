#pragma once

#include "ais.hpp"
#include "seq.hpp"

#include <string_view>
#include <unordered_map>

namespace seqmaker {
class Sequencer {
  private:
    std::unordered_map<ais::mmsi_t, ais::Trajectory> trajectories_{};

  protected:
    std::string_view delimiter_;   // NOLINT
    split_args split_args_;        // NOLINT

    void run(bool /* apply_low_pass_filter */) noexcept;

  public:
    explicit Sequencer(split_args /* split_args */, std::string_view /* delimiter */ = "");

    virtual ~Sequencer() = default;

    Sequencer(const Sequencer&) = default;

    Sequencer(Sequencer&&) = default;

    Sequencer& operator=(const Sequencer&) = default;

    Sequencer& operator=(Sequencer&&) = default;

    virtual void init(std::size_t /* n_trajectories */) noexcept = 0;

    virtual void process(ais::mmsi_t /* mmsi */,
                         const ais::Trajectory& /* trajectory */) noexcept = 0;

    void add_trajectory(ais::mmsi_t /* mmsi */, const ais::Trajectory& /* trajectory */) noexcept;
};
}   // namespace seqmaker