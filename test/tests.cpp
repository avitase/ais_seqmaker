#include "ais.hpp"
#include "seq.hpp"
#include "seq_maker.hpp"
#include "utility.hpp"

#include <catch2/catch.hpp>
#include <cmath>
#include <optional>
#include <string>

TEST_CASE("Test distance measure", "[ais]") {
    using namespace seqmaker;
    constexpr auto one_deg = 600000U;
    constexpr ais::Point p1{.latitude = 52 * one_deg, .longitude = 13 * one_deg};
    constexpr ais::Point p2{.latitude = 50 * one_deg, .longitude = 10 * one_deg};

    constexpr auto PI = 3.14159265359;
    const auto cos_mlat = std::cos(51. / 180. * PI);
    const auto dist_exp = std::sqrt(4 + cos_mlat * cos_mlat * 9) * 60;

    REQUIRE(p1.dist_nm(p2) == Approx{dist_exp});
}

TEST_CASE("Test MMSI validator", "[ais]") {
    using namespace seqmaker::ais;

    REQUIRE(is_valid_mmsi(200000000));
    REQUIRE(is_valid_mmsi(212345678));
    REQUIRE(is_valid_mmsi(799999999));
    REQUIRE(not is_valid_mmsi(199999999));
    REQUIRE(not is_valid_mmsi(800000000));

    REQUIRE(not is_valid_mmsi(21234567));
    REQUIRE(not is_valid_mmsi(2123456789));
    REQUIRE(not is_valid_mmsi(12345678));
    REQUIRE(not is_valid_mmsi(112345678));
    REQUIRE(not is_valid_mmsi(812345678));
    REQUIRE(not is_valid_mmsi(912345678));
}

TEST_CASE("Test adjacent difference", "[utility]") {
    using namespace seqmaker;
    constexpr std::array x{4, 8, 15, 16, 23, 42};
    constexpr std::array diff_expected{
        11,   // 4 - 15
        8,    // 8 - 16
        8,    // 15 - 23
        26,   // 16 - 42
    };

    constexpr unsigned stride = 2;
    std::vector<int> diff;
    diff.reserve(x.size() - stride);

    utility::adjacent_diff(
        x.begin(),
        x.end(),
        std::back_inserter(diff),
        [](auto a, auto b) { return b - a; },
        stride);

    REQUIRE(diff.size() == x.size() - stride);
    REQUIRE(diff.size() == diff_expected.size());
    for (auto i = 0U; i < diff.size(); i++) {
        REQUIRE(diff[i] == diff_expected[i]);
    }
}

TEST_CASE("Test estimation of recorded time", "[utility]") {
    const std::string T1{"123.4"};   // min 2, sec 3, msec 400
    const std::string T2{"173.6"};   // min 2, sec 53, msec 600

    const std::string slot1{"2"};
    const std::string slot2{"4"};
    const std::string slot3{"50"};
    const std::string slot4{"55"};

    using namespace seqmaker;
    const auto t11 = utility::time_recorded<ais::time_t>(T1, slot1);
    const auto t12 = utility::time_recorded<ais::time_t>(T1, slot2);
    const auto t13 = utility::time_recorded<ais::time_t>(T1, slot3);
    const auto t14 = utility::time_recorded<ais::time_t>(T1, slot4);

    const auto t21 = utility::time_recorded<ais::time_t>(T2, slot1);
    const auto t22 = utility::time_recorded<ais::time_t>(T2, slot2);
    const auto t23 = utility::time_recorded<ais::time_t>(T2, slot3);
    const auto t24 = utility::time_recorded<ais::time_t>(T2, slot4);

    REQUIRE(t11 == 122);
    REQUIRE(t21 == 182);
    REQUIRE(t12 == 124);
    REQUIRE(t22 == 184);
    REQUIRE(t13 == 110);
    REQUIRE(t23 == 170);
    REQUIRE(t14 == 115);
    REQUIRE(t24 == 175);
}

TEST_CASE("Test low pass filter", "[utility]") {
    using namespace seqmaker;
    auto filter = [](auto v) {
        auto last = utility::low_pass_filter(v.begin(), v.end(), v.begin(), [](auto a, auto b) {
            return std::abs(a - b) < 2;
        });
        v.erase(last, v.end());

        return v;
    };

    REQUIRE(filter(std::vector{1, 2, 3, 99, 5, 6, 7}) == std::vector{1, 2, 3, 5, 6, 7});
    REQUIRE(filter(std::vector{99, 2, 3, 4, 5, 6, 7}) == std::vector{2, 3, 4, 5, 6, 7});
    REQUIRE(filter(std::vector{1, 2, 3, 4, 5, 6, 99}) == std::vector{1, 2, 3, 4, 5, 6});
    REQUIRE(filter(std::vector{99, 2, 3, 4, 5, 6, 99}) == std::vector{2, 3, 4, 5, 6});
    REQUIRE(filter(std::vector{1, 2, 99, 99, 5, 6, 7}) == std::vector{1, 2, 99, 99, 5, 6, 7});
    REQUIRE(filter(std::vector{1, 2, 99, 4, 99, 6, 7}) == std::vector{1, 2, 6, 7});
    REQUIRE(filter(std::vector{1, 2, 99, 55, 5, 6, 7}) == std::vector{1, 2, 5, 6, 7});
}

TEST_CASE("Test interpolation", "[seq]") {
    using namespace seqmaker;
    auto make_pos = [](ais::time_t t, ais::Point::value_type lat, ais::Point::value_type lon) {
        return ais::Position{.t = t, .x = ais::Point{.latitude = lat, .longitude = lon}};
    };

    const ais::Trajectory trajectory{make_pos(0, 0, 0), make_pos(20, 4, 2), make_pos(50, 10, 5)};
    constexpr std::array seq_exptected{
        ais::Point{0, 0},    // t = 0
        ais::Point{2, 1},    // t = 10
        ais::Point{4, 2},    // t = 20
        ais::Point{6, 3},    // t = 30
        ais::Point{8, 4},    // t = 40
        ais::Point{10, 5},   // t = 50
    };

    const auto seq = interpolate(trajectory, seq_exptected.size(), 10);
    REQUIRE(seq.size() == seq_exptected.size());

    for (auto i = 0U; i < seq.size(); i++) {
        REQUIRE(seq[i].latitude == seq_exptected[i].latitude);
        REQUIRE(seq[i].longitude == seq_exptected[i].longitude);
    }
}

TEST_CASE("Test split", "[seq]") {
    using namespace seqmaker;
    auto make_pos = [](ais::time_t t, ais::Point::value_type lat, ais::Point::value_type lon) {
        return ais::Position{.t = t, .x = ais::Point{.latitude = lat, .longitude = lon}};
    };

    constexpr auto seq_length = 5U;
    constexpr auto dti = 5U;
    constexpr auto dt_max = 15U;
    constexpr auto ds_max = 5. / (600000. / 60.);

    const ais::Trajectory trajectory{
        make_pos(/*   t= */ 0, /*  lat= */ 0, /*  lon= */ 0),   // 1.1
        make_pos(/*  t= */ 10, /*  lat= */ 4, /*  lon= */ 2),   // 1.2
        make_pos(/*  t= */ 20, /*  lat= */ 8, /*  lon= */ 4),   // 1.3
        make_pos(/*  t= */ 30, /* lat= */ 12, /*  lon= */ 6),   // 1.x
        make_pos(/*  t= */ 40, /* lat= */ 16, /*  lon= */ 8),   // 2.1
        make_pos(/*  t= */ 50, /* lat= */ 20, /* lon= */ 10),   // 2.2
        make_pos(/*  t= */ 60, /* lat= */ 24, /* lon= */ 12),   // 2.3
        make_pos(/*  t= */ 70, /* lat= */ 28, /* lon= */ 14),   // 2.x
        make_pos(/* t= */ 999, /* lat= */ 32, /* lon= */ 16),   //
        make_pos(/*  t= */ 90, /* lat= */ 36, /* lon= */ 18),   // 3.1
        make_pos(/* t= */ 100, /* lat= */ 40, /* lon= */ 20),   // 3.2
        make_pos(/* t= */ 110, /* lat= */ 44, /* lon= */ 22),   // 3.3
        make_pos(/* t= */ 120, /* lat= */ 48, /* lon= */ 24),   // 3.x
        make_pos(/* t= */ 130, /* lat= */ 52, /* lon= */ 26),   //
        make_pos(/* t= */ 140, /* lat= */ 60, /* lon= */ 28),   //
        make_pos(/* t= */ 150, /* lat= */ 60, /* lon= */ 30),   //
    };

    constexpr std::array seq_expected{
        ais::Point{0, 0},    // t = 0
        ais::Point{2, 1},    // t = 5
        ais::Point{4, 2},    // t = 10
        ais::Point{6, 3},    // t = 15
        ais::Point{8, 4},    // t = 20
        ais::Point{10, 5},   // t = 25

        ais::Point{16, 8},    // t = 0
        ais::Point{18, 9},    // t = 5
        ais::Point{20, 10},   // t = 10
        ais::Point{22, 11},   // t = 15
        ais::Point{24, 12},   // t = 20
        ais::Point{26, 13},   // t = 25

        ais::Point{36, 18},   // t = 0
        ais::Point{38, 19},   // t = 5
        ais::Point{40, 20},   // t = 10
        ais::Point{42, 21},   // t = 15
        ais::Point{44, 22},   // t = 20
        ais::Point{46, 23},   // t = 25
    };

    constexpr split_args split_args{.seq_length = seq_length,
                                    .dt_max = dt_max,
                                    .dti = dti,
                                    .ds_max = ds_max,
                                    .v_min = 0.};
    const auto rate = drop_rate(trajectory, split_args);
    REQUIRE(rate == Approx(4. / static_cast<double>(trajectory.size())));   // NOLINT

    const auto seq = split(trajectory, split_args);
    REQUIRE(seq.size() == seq_expected.size());

    for (auto i = 0U; i < seq.size(); i++) {
        REQUIRE(seq[i].latitude == seq_expected[i].latitude);
        REQUIRE(seq[i].longitude == seq_expected[i].longitude);
    }
}

TEST_CASE("Test seqmaker with LPF", "[seqmaker]") {
    using namespace seqmaker;
    auto make_pos = [](ais::time_t t, ais::Point::value_type lat, ais::Point::value_type lon) {
        return ais::Position{.t = t, .x = ais::Point{.latitude = lat, .longitude = lon}};
    };

    constexpr auto seq_length = 5U;
    constexpr auto dti = 5U;
    constexpr auto dt_max = 15U;
    constexpr auto ds_max = 5. / (600000. / 60.);

    ais::Trajectory trajectory{
        make_pos(/* t=   */ 0, /* lat=   */ 0, /* lon=   */ 0),   // NOLINT // 1.1
        make_pos(/* t=  */ 10, /* lat=   */ 4, /* lon=   */ 2),   // NOLINT // 1.2
        make_pos(/* t=  */ 11, /* lat=  */ 99, /* lon=   */ 4),   // NOLINT
        make_pos(/* t=  */ 20, /* lat=   */ 8, /* lon=   */ 4),   // NOLINT // 1.3
        make_pos(/* t=  */ 30, /* lat=  */ 12, /* lon=   */ 6),   // NOLINT // 1.x
        make_pos(/* t=  */ 31, /* lat=  */ 16, /* lon=   */ 8),   // NOLINT
        make_pos(/* t=  */ 40, /* lat=  */ 16, /* lon=   */ 8),   // NOLINT
        make_pos(/* t=  */ 41, /* lat=  */ 20, /* lon=  */ 99),   // NOLINT
        make_pos(/* t=  */ 42, /* lat=  */ 20, /* lon=  */ 99),   // NOLINT
        make_pos(/* t=  */ 50, /* lat=  */ 20, /* lon=  */ 10),   // NOLINT // 2.1
        make_pos(/* t=  */ 60, /* lat=  */ 24, /* lon=  */ 12),   // NOLINT // 2.2
        make_pos(/* t=  */ 70, /* lat=  */ 28, /* lon=  */ 14),   // NOLINT // 2.3
        make_pos(/* t=  */ 80, /* lat=  */ 32, /* lon=  */ 16),   // NOLINT // 2.x
        make_pos(/* t=  */ 90, /* lat=  */ 36, /* lon=  */ 18),   // NOLINT // 3.1
        make_pos(/* t= */ 999, /* lat=  */ 36, /* lon=  */ 18),   // NOLINT
        make_pos(/* t= */ 100, /* lat=  */ 40, /* lon=  */ 20),   // NOLINT // 3.2
        make_pos(/* t= */ 110, /* lat=  */ 44, /* lon=  */ 22),   // NOLINT // 3.3
        make_pos(/* t= */ 120, /* lat=  */ 48, /* lon=  */ 24),   // NOLINT // 3.x
        make_pos(/* t= */ 130, /* lat=  */ 52, /* lon=  */ 26),   // NOLINT // 4.1
        make_pos(/* t= */ 140, /* lat=  */ 56, /* lon=  */ 28),   // NOLINT // 4.2
        make_pos(/* t= */ 150, /* lat=  */ 60, /* lon=  */ 30),   // NOLINT // 4.3
        make_pos(/* t= */ 160, /* lat=  */ 64, /* lon=  */ 32),   // NOLINT // 4.x
        make_pos(/* t= */ 161, /* lat=  */ 64, /* lon=  */ 99),   // NOLINT
        make_pos(/* t= */ 170, /* lat=  */ 68, /* lon=  */ 34),   // NOLINT
        make_pos(/* t= */ 180, /* lat=  */ 72, /* lon=  */ 36),   // NOLINT
        make_pos(/* t= */ 190, /* lat=  */ 76, /* lon=  */ 38),   // NOLINT
    };
    std::mt19937 g(0);   // NOLINT
    std::shuffle(trajectory.begin(), trajectory.end(), g);

    auto in_time = [](auto a, auto b) { return a.t < b.t; };
    REQUIRE(not std::is_sorted(trajectory.begin(), trajectory.end(), in_time));

    constexpr std::array seq_expected{
        ais::Point{0, 0},    // NOLINT // t = 0
        ais::Point{2, 1},    // NOLINT // t = 5
        ais::Point{4, 2},    // NOLINT // t = 10
        ais::Point{6, 3},    // NOLINT // t = 15
        ais::Point{8, 4},    // NOLINT // t = 20
        ais::Point{10, 5},   // NOLINT // t = 25

        ais::Point{20, 10},   // NOLINT // t = 0
        ais::Point{22, 11},   // NOLINT // t = 5
        ais::Point{24, 12},   // NOLINT // t = 10
        ais::Point{26, 13},   // NOLINT // t = 15
        ais::Point{28, 14},   // NOLINT // t = 20
        ais::Point{30, 15},   // NOLINT // t = 25

        ais::Point{36, 18},   // NOLINT // t = 0
        ais::Point{38, 19},   // NOLINT // t = 5
        ais::Point{40, 20},   // NOLINT // t = 10
        ais::Point{42, 21},   // NOLINT // t = 15
        ais::Point{44, 22},   // NOLINT // t = 20
        ais::Point{46, 23},   // NOLINT // t = 25

        ais::Point{52, 26},   // NOLINT // t = 0
        ais::Point{54, 27},   // NOLINT // t = 5
        ais::Point{56, 28},   // NOLINT // t = 10
        ais::Point{58, 29},   // NOLINT // t = 15
        ais::Point{60, 30},   // NOLINT // t = 20
        ais::Point{62, 31},   // NOLINT // t = 25
    };

    constexpr ais::mmsi_t MMSI = 200000000;
    static_assert(ais::is_valid_mmsi(MMSI));

    auto seq_maker = SequenceMaker{split_args{.seq_length = seq_length,
                                              .dt_max = dt_max,
                                              .dti = dti,
                                              .ds_max = ds_max,
                                              .v_min = 0.},
                                   ""};
    seq_maker.add_trajectory(MMSI, trajectory);
    for (auto [mmsi, seq] : seq_maker.run(true)) {
        REQUIRE(mmsi == MMSI);

        REQUIRE(seq.size() == seq_expected.size());

        for (auto i = 0U; i < seq.size(); i++) {
            REQUIRE(seq[i].latitude == seq_expected[i].latitude);
            REQUIRE(seq[i].longitude == seq_expected[i].longitude);
        }
    }
}
