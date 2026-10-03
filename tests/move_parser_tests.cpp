#include "move_parser.h"

#include <gtest/gtest.h>

#include <string_view>

using namespace cj::pente;
using namespace cj::pente::move_parser;

namespace
{
    void expect_parsed_move(
        const std::expected<coord, move_parse_error>& result,
        coord expectedPosition)
    {
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->x, expectedPosition.x);
        EXPECT_EQ(result->y, expectedPosition.y);
    }
}

TEST(move_parser, parses_centre_in_either_case)
{
    expect_parsed_move(parse_move_from_string("O"), {BoardCentre, BoardCentre});
    expect_parsed_move(parse_move_from_string("o"), {BoardCentre, BoardCentre});
}

TEST(move_parser, parses_single_direction_and_zero_distance)
{
    expect_parsed_move(parse_move_from_string("R3"), {BoardCentre + 3, BoardCentre});
    expect_parsed_move(parse_move_from_string("u2"), {BoardCentre, BoardCentre - 2});
    expect_parsed_move(parse_move_from_string("L0"), {BoardCentre, BoardCentre});
}

TEST(move_parser, parses_horizontal_and_vertical_components_in_either_order)
{
    expect_parsed_move(parse_move_from_string("R3U2"), {BoardCentre + 3, BoardCentre - 2});
    expect_parsed_move(parse_move_from_string("d4l1"), {BoardCentre - 1, BoardCentre + 4});
    expect_parsed_move(parse_move_from_string("U2R3"), {BoardCentre + 3, BoardCentre - 2});
}

TEST(move_parser, accepts_moves_ending_at_board_edges)
{
    expect_parsed_move(parse_move_from_string("R9D9"), {GridSize - 1, GridSize - 1});
    expect_parsed_move(parse_move_from_string("L9U9"), {0, 0});
}

TEST(move_parser, rejects_empty_and_invalid_direction)
{
    const auto empty = parse_move_from_string("");
    ASSERT_FALSE(empty.has_value());
    EXPECT_EQ(empty.error(), move_parse_error::invalid_format);

    const auto invalidDirection = parse_move_from_string("X2");
    ASSERT_FALSE(invalidDirection.has_value());
    EXPECT_EQ(invalidDirection.error(), move_parse_error::invalid_format);
}

TEST(move_parser, rejects_missing_distance)
{
    for (const auto input : {std::string_view{"R"}, std::string_view{"U2D"}})
    {
        const auto result = parse_move_from_string(input);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), move_parse_error::invalid_format);
    }
}

TEST(move_parser, rejects_signed_distances)
{
    for (const auto input : {
             std::string_view{"R-1"}, std::string_view{"U-1"},
             std::string_view{"R1U-2"}, std::string_view{"L+1"}})
    {
        const auto result = parse_move_from_string(input);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), move_parse_error::invalid_format);
    }
}

TEST(move_parser, rejects_repeating_an_axis)
{
    for (const auto input : {std::string_view{"R1L1"}, std::string_view{"U1D1"}})
    {
        const auto result = parse_move_from_string(input);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), move_parse_error::invalid_format);
    }
}

TEST(move_parser, rejects_components_after_both_axes)
{
    const auto result = parse_move_from_string("R1U1D999999999999999999999999999999999");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), move_parse_error::invalid_format);
}

TEST(move_parser, rejects_out_of_bounds_moves_in_each_direction)
{
    for (const auto input : {
             std::string_view{"U10"}, std::string_view{"D10"},
             std::string_view{"L10"}, std::string_view{"R10"}})
    {
        const auto result = parse_move_from_string(input);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), move_parse_error::out_of_bounds);
    }
}

TEST(move_parser, rejects_distances_that_overflow_size_type)
{
    const auto result = parse_move_from_string("R999999999999999999999999999999999999999");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), move_parse_error::out_of_bounds);
}
