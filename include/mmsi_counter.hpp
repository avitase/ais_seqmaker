#pragma once

#include "ais.hpp"

#include <string_view>
#include <utility>
#include <vector>

namespace seqmaker {
[[nodiscard]] std::vector<std::pair<ais::mmsi_t, std::size_t>>
    count_mmsi(std::string_view /* delimiter_ */);
}   // namespace seqmaker
