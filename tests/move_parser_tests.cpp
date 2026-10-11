#include "move_parser.h"

#include <gtest/gtest.h>

#include <string_view>

using namespace cj::pente;
using namespace cj::pente::move_parser;

namespace
{
    void test_valid_move(std::string_view input, coord expectedPosition)
    {
        const auto result = parse_move_from_string(input);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->x, expectedPosition.x);
        EXPECT_EQ(result->y, expectedPosition.y);
    }

    void test_invalid_move(std::string_view input, move_parse_error expectedError)
    {
        const auto result = parse_move_from_string(input);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), expectedError);
    }
}

TEST(move_parser, parses_centre_in_either_case)
{
    test_valid_move("O", {BoardCentre, BoardCentre});
    test_valid_move("o", {BoardCentre, BoardCentre});
}

TEST(move_parser, parses_single_direction_and_zero_distance)
{
    test_valid_move("R3", {BoardCentre + 3, BoardCentre});
    test_valid_move("u2", {BoardCentre, BoardCentre - 2});
    test_valid_move("L0", {BoardCentre, BoardCentre});
}

TEST(move_parser, parses_horizontal_and_vertical_components_in_either_order)
{
    test_valid_move("R3U2", {BoardCentre + 3, BoardCentre - 2});
    test_valid_move("d4l1", {BoardCentre - 1, BoardCentre + 4});
    test_valid_move("U2R3", {BoardCentre + 3, BoardCentre - 2});
}

TEST(move_parser, accepts_moves_ending_at_board_edges)
{
    test_valid_move("R9D9", {GridSize - 1, GridSize - 1});
    test_valid_move("L9U9", {0, 0});
}

TEST(move_parser, rejects_empty_and_invalid_direction)
{
    test_invalid_move("", move_parse_error::invalid_format);

    test_invalid_move("X2", move_parse_error::invalid_format);
}

TEST(move_parser, rejects_missing_distance)
{
    test_invalid_move("R", move_parse_error::invalid_format);
    test_invalid_move("U2D", move_parse_error::invalid_format);
}

TEST(move_parser, rejects_signed_distances)
{
    test_invalid_move("R-1", move_parse_error::invalid_format);
    test_invalid_move("U-1", move_parse_error::invalid_format);
    test_invalid_move("R1U-2", move_parse_error::invalid_format);
    test_invalid_move("L+1", move_parse_error::invalid_format);
}

TEST(move_parser, rejects_repeating_an_axis)
{
    test_invalid_move("R1L1", move_parse_error::invalid_format);
    test_invalid_move("U1D1", move_parse_error::invalid_format);
}

TEST(move_parser, rejects_components_after_both_axes)
{
    test_invalid_move("R1U1D999999999999999999999999999999999", move_parse_error::invalid_format);
}

TEST(move_parser, rejects_out_of_bounds_moves_in_each_direction)
{
    test_invalid_move("U10", move_parse_error::out_of_bounds);
    test_invalid_move("D10", move_parse_error::out_of_bounds);
    test_invalid_move("L10", move_parse_error::out_of_bounds);
    test_invalid_move("R10", move_parse_error::out_of_bounds);
}

TEST(move_parser, rejects_distances_that_overflow_size_type)
{
    test_invalid_move("R999999999999999999999999999999999999999", move_parse_error::out_of_bounds);
}
