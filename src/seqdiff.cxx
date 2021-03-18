#include "ais.hpp"
#include "argparse.hpp"
#include "seq_diff.hpp"
#include "utility.hpp"

#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

static constexpr auto USAGE = R"(seqdiff

    Determines adjacent differences of time and position of AIS data with a common MMSI, where the
    data stream is read from standard input.

    The first five columns of the input data are interpreted as
     (1) Time of AIS message reception as UTC epoch, e.g., 1456786800.005
     (2) MMSI
     (3) AIS slot second
     (4) Latitude
     (5) Longitude

    The result is stored as a binary stream of two signed 32 bit integers, representing the
    pairwise temporal and spatial differences, respectively, in a given file. The spatial difference
    is given in seconds, the spatial difference in 1/10000 nautical miles.

    Example:
        $ cat my_data.csv | ./seqdiff -s 10 -d ", " -f "dump.bin"
        Above command determines the adjacent differences with a stride of 10. The column separator
        is ", " and the binary data are dumped to the file "dump.bin".

    Options:
        -h                Prints this message.
        -s [stride]       The stride (default 1).
        -d "[delimiter]"  The delimiter used to separate columns (default ", ").
        -f                The name of the output file for the binary data.)";

static constexpr auto ARG_d_DEFAULT = ", ";
static constexpr auto ARG_s_DEFAULT = "1";

[[nodiscard]] auto strip_quotes(std::string str) noexcept {
    if (auto n = str.size(); n > 2 and str.starts_with('\"') and str.ends_with('\"')) {
        str = str.erase(0, 1);
        str = str.erase(n - 2, n - 1);
    }
    return str;
}

void dump_seq(
    const std::vector<std::pair<seqmaker::ais::time_t, seqmaker::ais::Point::value_type>>& seq,
    const std::filesystem::path& path) {
    std::ofstream f(path, std::ios::binary);
    for (auto p : seq) {
        constexpr auto Nt = sizeof(decltype(p.first));
        constexpr auto Nx = sizeof(decltype(p.second));

        std::array<char, Nt + Nx> bytes;   // NOLINT

        // cppcheck-suppress containerOutOfBounds
        std::memcpy(&bytes[0], &(p.first), Nt);   // NOLINT

        // cppcheck-suppress containerOutOfBounds
        std::memcpy(&bytes[Nx], &(p.second), Nx);   // NOLINT

        // cppcheck-suppress containerOutOfBounds
        f.write(&bytes[0], Nt + Nx);   // NOLINT
    }
}

int main(int argc, const char** argv) {
    using namespace seqmaker;

    argparse::Argparse args{argc, argv};
    if (auto zero_args = (args.n_args() == 0); zero_args or args.is_set("-h")) {
        std::cout << USAGE << '\n';
        return zero_args ? 1 : 0;
    }

    if (auto invalid_arg = args.check_args(std::set<std::string>{"-s", "-d", "-f"}); invalid_arg) {
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
        const auto s = utility::to<int>(args.get("-s").value_or(ARG_s_DEFAULT), 0);
        const auto f = std::filesystem::path{strip_quotes(args.get("-f").value_or(""))};

        if (s <= 0) {
            std::cerr << "Error: Value of -s has to be non-zero and positive\n";
            return 1;
        }

        if (f.empty()) {
            std::cerr << "Error: Value of -f has to be a valid file name\n";
            return 1;
        }

        const auto us = static_cast<unsigned>(s);
        dump_seq(SequenceDiff{d}.run(us), f);
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}
