#include <gtest/gtest.h>

#include "../src/rescale.hpp"
#include <alpha-lib/overlay_alpha.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

#if ENABLE_NEON

TEST(overlay_alpha, rescale_div256_floor) {
    std::vector<uint16_t> in(1ul << 16);
    std::iota(std::begin(in), std::end(in),
              std::numeric_limits<uint16_t>::min());
    std::vector<uint8_t> expected(in.size());
    std::vector<uint8_t> result(in.size());

    for (size_t i = 0; i < in.size(); i += 8) {
        vst1_u8(&result[i], div256_floor(vld1q_u16(&in[i])));
        for (size_t j = 0; j < 8; ++j) {
            expected[i + j] = in[i + j] / 256;
        }
    }
    EXPECT_EQ(result, expected);
}

TEST(overlay_alpha, rescale_div256_round) {
    std::vector<uint16_t> in(1ul << 16);
    std::iota(std::begin(in), std::end(in),
              std::numeric_limits<uint16_t>::min());
    std::vector<uint8_t> expected(in.size());
    std::vector<uint8_t> result(in.size());

    for (size_t i = 0; i < in.size(); i += 8) {
        vst1_u8(&result[i], div256_round(vld1q_u16(&in[i])));
        for (size_t j = 0; j < 8; ++j) {
            expected[i + j] = std::round(in[i + j] / 256.);
        }
    }
    EXPECT_EQ(result, expected);
}

TEST(overlay_alpha, rescale_div255_floor) {
    std::vector<uint16_t> in(1ul << 16);
    std::iota(std::begin(in), std::end(in),
              std::numeric_limits<uint16_t>::min());
    std::vector<uint8_t> expected(in.size());
    std::vector<uint8_t> result(in.size());

    for (size_t i = 0; i < in.size(); i += 8) {
        vst1_u8(&result[i], div255_floor(vld1q_u16(&in[i])));
        for (size_t j = 0; j < 8; ++j) {
            expected[i + j] = in[i + j] / 255;
        }
    }
    EXPECT_EQ(result, expected);
}

TEST(overlay_alpha, rescale_div255_round) {
    std::vector<uint16_t> in((1ul << 16) - 128);
    std::iota(std::begin(in), std::end(in),
              std::numeric_limits<uint16_t>::min());
    std::vector<uint8_t> expected(in.size());
    std::vector<uint8_t> result(in.size());

    for (size_t i = 0; i < in.size(); i += 8) {
        vst1_u8(&result[i], div255_round(vld1q_u16(&in[i])));
        for (size_t j = 0; j < 8; ++j) {
            expected[i + j] = std::round(in[i + j] / 255.);
        }
    }
    EXPECT_EQ(result, expected);
}

TEST(overlay_alpha, rescale_div255_round_approx) {
    std::vector<uint16_t> in(1ul << 16);
    std::iota(std::begin(in), std::end(in),
              std::numeric_limits<uint16_t>::min());
    std::vector<uint8_t> expected(in.size());
    std::vector<uint8_t> result(in.size());

    for (size_t i = 0; i < in.size(); i += 8) {
        vst1_u8(&result[i], div255_round_approx(vld1q_u16(&in[i])));
        for (size_t j = 0; j < 8; ++j) {
            expected[i + j] = std::round(in[i + j] / 255.);
        }
    }
    std::vector<int8_t> differences(in.size());
    std::transform(std::begin(expected), std::end(expected), std::begin(result),
                   differences.begin(), std::minus<>());
    auto errors = std::count_if(differences.begin(), differences.end(),
                                [](int8_t d) { return d != 0; });
    EXPECT_LE(errors, 127);
    EXPECT_LE(*std::max_element(differences.begin(), differences.end()), 0);
    EXPECT_GE(*std::min_element(differences.begin(), differences.end()), -1);
}

#endif

TEST(overlay_alpha, rescale_div256_floor_1) {
    std::vector<uint16_t> in(1ul << 16);
    std::iota(std::begin(in), std::end(in),
              std::numeric_limits<uint16_t>::min());
    std::vector<uint8_t> expected(in.size());
    std::vector<uint8_t> result(in.size());

    for (size_t i = 0; i < in.size(); i += 1) {
        result[i]   = div256_floor(in[i]);
        expected[i] = in[i] / 256;
    }
    EXPECT_EQ(result, expected);
}

TEST(overlay_alpha, rescale_div256_round_1) {
    std::vector<uint16_t> in((1ul << 16) - 128);
    std::iota(std::begin(in), std::end(in),
              std::numeric_limits<uint16_t>::min());
    std::vector<uint8_t> expected(in.size());
    std::vector<uint8_t> result(in.size());

    for (size_t i = 0; i < in.size(); i += 1) {
        result[i]   = div256_round(in[i]);
        expected[i] = std::round(in[i] / 256.);
    }
    EXPECT_EQ(result, expected);
}

TEST(overlay_alpha, rescale_div255_floor_1) {
    std::vector<uint16_t> in(1ul << 16);
    std::iota(std::begin(in), std::end(in),
              std::numeric_limits<uint16_t>::min());
    std::vector<uint8_t> expected(in.size());
    std::vector<uint8_t> result(in.size());

    for (size_t i = 0; i < in.size(); i += 1) {
        result[i]   = div255_floor(in[i]);
        expected[i] = in[i] / 255;
    }
    EXPECT_EQ(result, expected);
}

TEST(overlay_alpha, rescale_div255_round_1) {
    std::vector<uint16_t> in((1ul << 16) - 128);
    std::iota(std::begin(in), std::end(in),
              std::numeric_limits<uint16_t>::min());
    std::vector<uint8_t> expected(in.size());
    std::vector<uint8_t> result(in.size());

    for (size_t i = 0; i < in.size(); i += 1) {
        result[i]   = div255_round(in[i]);
        expected[i] = std::round(in[i] / 255.);
    }
    EXPECT_EQ(result, expected);
}

TEST(overlay_alpha, rescale_div255_round_approx_1) {
    std::vector<uint16_t> in((1ul << 16) - 128);
    std::iota(std::begin(in), std::end(in),
              std::numeric_limits<uint16_t>::min());
    std::vector<uint8_t> expected(in.size());
    std::vector<uint8_t> result(in.size());

    for (size_t i = 0; i < in.size(); i += 1) {
        result[i]   = div255_round_approx(in[i]);
        expected[i] = std::round(in[i] / 255.);
    }
    std::vector<int8_t> differences(in.size());
    std::transform(std::begin(expected), std::end(expected), std::begin(result),
                   differences.begin(), std::minus<>());
    auto errors = std::count_if(differences.begin(), differences.end(),
                                [](int8_t d) { return d != 0; });
    EXPECT_LE(errors, 127);
    EXPECT_LE(*std::max_element(differences.begin(), differences.end()), 0);
    EXPECT_GE(*std::min_element(differences.begin(), differences.end()), -1);
}
