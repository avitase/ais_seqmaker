#include "ais.hpp"
#include "argparse.hpp"
#include "mmsi_counter.hpp"
#include "seq_counter.hpp"
#include "seq_maker.hpp"
#include "utility.hpp"

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

static constexpr auto USAGE = R"(seqmaker

    Gathers lines of AIS data from standard input as sequences by MMSI.
    Sequences of a common MMSI are split by length and if consecutive points deviate significantly.
    The resulting sequences are split until they have the target length. Remaining parts are
    discarded.

    The first five columns of the input data are interpreted as:
     (1) Time of AIS message reception as UTC epoch, e.g., 1456786800.005
     (2) MMSI
     (3) AIS slot second
     (4) Latitude
     (5) Longitude

    Sequences are interpolated and written to file, where the respective MMSI is used as file name.
    A line of AIS data is transformed into two signed 32 bit integers, representing latitude and
    longitude (both given in 1/10000 min). These integers are concatenated and stored binary.

    Additionally, the passed options and parameters of the last invocation are stored in the file
    args.txt and saved next to the generated binary files.

    Example:
        $ cat my_data.csv | ./seqmaker -d ", " -N 360 -t 50 -s .5 -i 10
        Above command splits AIS data in sequences with 361 pairs of latitude and longitude each,
        corresponding to a sequence length of 1h. Within a sequence, two consecutive points deviate
        less than 50 seconds (in the original AIS data) and 0.5 nm. The column separator is ", ".

    Options:
        -h                Prints this message.
        -c                Only count MMSI occurences and suppress generation of args.txt.
        -S                Suppress generation of files and print drop-rate of selection.
        -d "[delimiter]"  The delimiter used to separate columns (default ", ").
        -N [number]       Sequence length N, corresponding to a temporal duration of
                          N x interpolation length (cf. argument -i) and N + 1 grid points
                          (default 3600).
        -t [seconds]      Temporal threshold to split sequence (default 60 seconds).
        -s [nm]           Spatial threshold to split sequence (default .1 nautical miles).
        -i [seconds]      Interpolation length in seconds (default 6).
        -p [dir]          Parent directories for generated files (default ./).
        -l                Apply simple one-step low pass filter using given spatial threshold.
        -v [kt]           Minimal average speed in kt on interpolated sequence (default 0 kt).
)";

static constexpr auto ARG_d_DEFAULT = ", ";
static constexpr auto ARG_N_DEFAULT = "3600";
static constexpr auto ARG_t_DEFAULT = "60";
static constexpr auto ARG_s_DEFAULT = ".1";
static constexpr auto ARG_i_DEFAULT = "6";
static constexpr auto ARG_p_DEFAULT = "";
static constexpr auto ARG_v_DEFAULT = "0.";

[[nodiscard]] auto strip_quotes(std::string str) noexcept {
    if (auto n = str.size(); n > 2 and str.starts_with('\"') and str.ends_with('\"')) {
        str = str.erase(0, 1);
        str = str.erase(n - 2, n - 1);
    }
    return str;
}

void dump_args(std::string_view delimiter,
               unsigned seq_length,
               unsigned dt_max,
               double ds_max,
               unsigned dti,
               double v,
               bool lpf,
               const std::filesystem::path& path) {
    std::ofstream f(path / "args.txt");
    f << "-d " << delimiter << ' ';
    f << "-N " << seq_length << ' ';
    f << "-t " << dt_max << ' ';
    f << "-s " << ds_max << ' ';
    f << "-i " << dti << ' ';
    f << "-v " << v << ' ';
    if (lpf) {
        f << "-l ";
    }
    f << "-p " << path << '\n';
}

void dump_seq(seqmaker::ais::mmsi_t mmsi,
              const std::vector<seqmaker::ais::Point>& seq,
              const std::filesystem::path& path) {
    const auto fname = (path / std::to_string(mmsi)).concat(".bin");
    std::ofstream f(fname, std::ios::binary);
    for (auto p : seq) {
        constexpr auto N = sizeof(decltype(p)::value_type);

        std::array<char, 2 * N> bytes;   // NOLINT

        // cppcheck-suppress containerOutOfBounds
        std::memcpy(&bytes[0], &(p.latitude), N);   // NOLINT

        // cppcheck-suppress containerOutOfBounds
        std::memcpy(&bytes[N], &(p.longitude), N);   // NOLINT

        // cppcheck-suppress containerOutOfBounds
        f.write(&bytes[0], 2 * N);   // NOLINT
    }
}

int main(int argc, const char** argv) {
    using namespace seqmaker;

    argparse::Argparse args{argc, argv};
    if (auto zero_args = (args.n_args() == 0); zero_args or args.is_set("-h")) {
        std::cout << USAGE << '\n';
        return zero_args ? 1 : 0;
    }

    if (auto invalid_arg = args.check_args(
            std::set<std::string>{"-c", "-S", "-d", "-N", "-t", "-s", "-i", "-l", "-p", "-v"});
        invalid_arg) {
        std::cout << "Unknown argument \"" << *invalid_arg << "\".\n";
        std::cout << "Use -h to print help.\n";
        return 1;
    }

    auto d = strip_quotes(args.get("-d").value_or(std::string{ARG_d_DEFAULT}));
    auto replace_char = [](auto str, const std::string& target, char replacement) {
        if (auto i = str.find(target); i != std::string::npos) {
            str.replace(i, 2, std::string{replacement});
        }
        return str;
    };
    d = replace_char(d, "\\t", '\t');

    try {
        if (args.is_set("-c")) {
            for (auto [mmsi, n] : count_mmsi(d)) {
                std::cout << mmsi << ": " << n << '\n';
            }
            return 0;
        }

        /*
         * TODO: workaround until compiler support std::from_chars for double
         */
        auto str2d = [](const std::string& str, double fallback) {
            try {
                return std::stod(str);
            } catch (const std::invalid_argument&) {
                return fallback;
            }
        };

        const auto N = utility::to<int>(args.get("-N").value_or(ARG_N_DEFAULT), 0);
        const auto t = utility::to<int>(args.get("-t").value_or(ARG_t_DEFAULT), 0);
        const auto i = utility::to<int>(args.get("-i").value_or(ARG_i_DEFAULT), 0);
        const auto s = str2d(args.get("-s").value_or(ARG_s_DEFAULT), 0.);
        const auto v = str2d(args.get("-v").value_or(ARG_v_DEFAULT), -1.);
        const auto lpf = args.is_set("-l");
        const auto p = std::filesystem::path{strip_quotes(args.get("-p").value_or(ARG_p_DEFAULT))};

        if (N <= 0) {
            std::cerr << "Error: Value of -N has to be non-zero and positive\n";
            return 1;
        }

        if (t <= 0) {
            std::cerr << "Error: Value of -t has to be non-zero and positive\n";
            return 1;
        }

        if (i <= 0) {
            std::cerr << "Error: Value of -i has to be non-zero and positive\n";
            return 1;
        }

        if (s <= 0.) {
            std::cerr << "Error: Value of -s has to be non-zero and positive\n";
            return 1;
        }

        if (v < 0.) {
            std::cerr << "Error: Value of -v has to be zero or positive\n";
            return 1;
        }

        const auto uN = static_cast<unsigned>(N);
        const auto ut = static_cast<unsigned>(t);
        const auto ui = static_cast<unsigned>(i);

        if (not p.empty()) {
            std::filesystem::create_directory(p);
        }
        dump_args(args.get("-d").value_or(std::string{ARG_d_DEFAULT}), uN, ut, s, ui, v, lpf, p);

        const split_args split_args{.seq_length = uN,
                                    .dt_max = ut,
                                    .dti = ui,
                                    .ds_max = s,
                                    .v_min = v};
        if (args.is_set("-S")) {
            if (v > 0.) {
                std::cerr << "Error: Option -S is incompatible with v > 0.\n";
                return 1;
            }
            for (auto [mmsi, drop_rate] : SequenceCounter{split_args, d}.run(lpf)) {
                std::cout << mmsi << ": " << drop_rate << '\n';
            }
        } else {
            for (auto [mmsi, seq] : SequenceMaker{split_args, d}.run(lpf)) {
                dump_seq(mmsi, seq, p);
            }
        }
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
