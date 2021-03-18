#include "mmsi_counter.hpp"

#include "io.hpp"
#include "utility.hpp"

#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace seqmaker {
[[nodiscard]] std::vector<std::pair<ais::mmsi_t, std::size_t>>
count_mmsi(std::string_view delimiter) {
    std::unordered_map<ais::mmsi_t, unsigned> hist;

    io::process_input_stream([delimiter, &hist](std::string_view line) {
        constexpr ais::mmsi_t fallback = 0;
        const auto mmsi = utility::split_map(
            line,
            delimiter,
            [](std::string_view /* t */, std::string_view mmsi) {
                return mmsi.empty() ? fallback : utility::to<ais::mmsi_t>(mmsi, fallback);
            });
        if (mmsi > 0) {
            auto it = hist.find(mmsi);
            if (it == hist.end()) {
                it = hist.emplace(mmsi, 0U).first;
            }
            it->second++;
        }
    });

    std::vector<std::pair<ais::mmsi_t, std::size_t>> sorted_counts;
    sorted_counts.reserve(hist.size());
    for (auto [mmsi, n] : hist) {
        sorted_counts.emplace_back(mmsi, n);
    }
    std::sort(sorted_counts.begin(), sorted_counts.end(), [](auto a, auto b) {
        return a.second > b.second;
    });

    return sorted_counts;
}
}   // namespace seqmaker
