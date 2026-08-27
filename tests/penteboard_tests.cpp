#include "penteboard.h"

#include <gtest/gtest.h>

using namespace cj::pente;
using namespace std;

namespace
{
    /// @brief Produces a predictable space value based on the coordinates, for testing purposes.
    /// @param x The x-coordinate.
    /// @param y The y-coordinate.
    /// @return Diagonal coordinates produce Empty, coordinates above the diagonal produce Black, and coordinates below the diagonal produce White.
    space space_from_coords(size_t x, size_t y)
    {
        if (x == y)
        {
            return space::Empty;
        }

        if (x < y)
        {
            return space::Black;
        }

        return space::White;
    }

    coord to_coord(size_t index)
    {
        return coord{index % GridSize, index / GridSize};
    }
}

// Tests for basic piece and space conversions.
TEST(penteboard, opposite_and_to_space)
{
    EXPECT_EQ(opposite(piece::Black), piece::White);
    EXPECT_EQ(opposite(piece::White), piece::Black);

    EXPECT_EQ(to_space(piece::Black), space::Black);
    EXPECT_EQ(to_space(piece::White), space::White);
}

// In and out of bounds checks for coordinates.
TEST(penteboard, is_in_bounds)
{
    EXPECT_TRUE(is_in_bounds(coord{0, 0}));
    EXPECT_TRUE(is_in_bounds(coord{BoardCentre, BoardCentre}));
    EXPECT_TRUE(is_in_bounds(coord{GridSize - 1, GridSize - 1}));

    EXPECT_FALSE(is_in_bounds(coord{GridSize, 0}));
    EXPECT_FALSE(is_in_bounds(coord{0, GridSize}));
    EXPECT_FALSE(is_in_bounds(signed_coord{-1, 0}));
    EXPECT_FALSE(is_in_bounds(signed_coord{0, -1}));
    EXPECT_FALSE(is_in_bounds(signed_coord{0, GridSize}));
    EXPECT_FALSE(is_in_bounds(signed_coord{GridSize, 0}));
}

// b.rows() should produce the same values as b[x, y] for all valid coordinates.
TEST(penteboard, get_rows_matches_indexing)
{
    board b{};

    auto coords = std::views::iota(0zu, GridSize * GridSize) | std::views::transform(to_coord);
        
    for (const auto& position : coords)
    {
        b[position] = space_from_coords(position.x, position.y);
    }

    size_t y = 0;
    for (auto row : b.rows())
    {
        EXPECT_EQ(row.size(), GridSize);

        for (size_t x = 0; x < GridSize; x++)
        {
            EXPECT_EQ(row[x], space_from_coords(x, y));
            EXPECT_EQ(row[x], (b[x, y]));
        }

        y++;
    }

    EXPECT_EQ(y, GridSize);
}

// A horizontal capture should remove the two captured pieces and return the number of pieces captured.
TEST(penteboard, apply_captures_horizontal)
{
    board b{};

    b[2, 5] = space::Black;
    b[3, 5] = space::White;
    b[4, 5] = space::White;
    b[5, 5] = space::Black;

    const auto captured = apply_pair_captures(b, {2, 5});

    EXPECT_EQ(captured, 2u);
    EXPECT_EQ((b[3, 5]), space::Empty);
    EXPECT_EQ((b[4, 5]), space::Empty);
}

// A double-direction capture should remove the four captured pieces and return the number of pieces captured.
TEST(penteboard, apply_captures_double_direction)
{
    board b{};

    b[2, 8] = space::Black;
    b[3, 8] = space::White;
    b[4, 8] = space::White;
        
    // placed piece
    b[5, 8] = space::Black;
        
    b[6, 8] = space::White;
    b[7, 8] = space::White;
    b[8, 8] = space::Black;

    const auto captured = apply_pair_captures(b, {5, 8});

    EXPECT_EQ(captured, 4u);
    EXPECT_EQ((b[3, 8]), space::Empty);
    EXPECT_EQ((b[4, 8]), space::Empty);
    EXPECT_EQ((b[6, 8]), space::Empty);
    EXPECT_EQ((b[7, 8]), space::Empty);
}

// A diagonal capture should remove the two captured pieces and return the number of pieces captured.
TEST(penteboard, apply_captures_diagonal)
{
    board b{};

    b[4, 4] = space::White;
    b[5, 5] = space::Black;
    b[6, 6] = space::Black;
    b[7, 7] = space::White;

    const auto captured = apply_pair_captures(b, {4, 4});

    EXPECT_EQ(captured, 2u);
    EXPECT_EQ((b[5, 5]), space::Empty);
    EXPECT_EQ((b[6, 6]), space::Empty);
}

// A capture should not occur if the played piece is not part of a valid capture pattern.
TEST(penteboard, apply_captures_no_capture)
{
    board b{};

    // scenario 1
    b[0, 0] = space::Black;
    b[1, 0] = space::White;

    const auto captured = apply_pair_captures(b, {0, 0});

    EXPECT_EQ(captured, 0u);
    EXPECT_EQ((b[1, 0]), space::White);

    // scenario 2
    b[0, 2] = space::Black;
    b[1, 2] = space::White;
    b[2, 2] = space::White;

    const auto captured2 = apply_pair_captures(b, {0, 2});
    
    EXPECT_EQ(captured2, 0u);
    EXPECT_EQ((b[1, 2]), space::White);
    EXPECT_EQ((b[2, 2]), space::White);
}

// Applying captures out of bounds should not capture any pieces.
TEST(penteboard, apply_captures_out_of_bounds)
{
    board b{};

    const auto captured = apply_pair_captures(b, {20, 20});

    EXPECT_EQ(captured, 0u);
}

// Adding captured pieces should accumulate correctly without reaching the win threshold.
TEST(penteboard, add_captured_pieces_accumulates_for_black_without_win)
{
    capture_pots pots{};

    const auto [totalCapturedPieces, hasWonByCaptures] = add_captured_pieces(pots, piece::Black, 2u);

    EXPECT_EQ(totalCapturedPieces, 2u);
    EXPECT_FALSE(hasWonByCaptures);
    EXPECT_EQ(pots.pieces_captured_by_black, 2u);
    EXPECT_EQ(pots.pieces_captured_by_white, 0u);
}

// Adding enough captured pieces should reach the capture win threshold.
TEST(penteboard, add_captured_pieces_reaches_capture_win_threshold)
{
    capture_pots pots{};
    pots.pieces_captured_by_white = 8u;

    const auto [totalCapturedPieces, hasWonByCaptures] = add_captured_pieces(pots, piece::White, 2u);

    EXPECT_EQ(totalCapturedPieces, 10u);
    EXPECT_TRUE(hasWonByCaptures);
    EXPECT_EQ(pots.pieces_captured_by_black, 0u);
    EXPECT_EQ(pots.pieces_captured_by_white, 10u);
}

// Adding captured pieces should correctly handle surpassing the capture win threshold.
TEST(penteboard, add_captured_pieces_passes_capture_win_threshold)
{
    capture_pots pots{};
    pots.pieces_captured_by_white = 8u;

    const auto [totalCapturedPieces, hasWonByCaptures] = add_captured_pieces(pots, piece::White, 4u);

    EXPECT_EQ(totalCapturedPieces, 12u);
    EXPECT_TRUE(hasWonByCaptures);
    EXPECT_EQ(pots.pieces_captured_by_black, 0u);
    EXPECT_EQ(pots.pieces_captured_by_white, 12u);
}

// Checking for five in a row horizontally.
TEST(penteboard, check_five_in_a_row_horizontal)
{
    board b{};

    for (size_t x = 3; x <= 7; ++x)
    {
        b[x, 9] = space::Black;
    }

    // placed piece is central in the five-in-a-row
    EXPECT_TRUE(check_five_in_a_row(b, {5, 9}));
}

// Checking for five in a row diagonally.
TEST(penteboard, check_five_in_a_row_diagonal)
{
    board b{};

    for (size_t i = 0; i < 5; ++i)
    {
        b[2 + i, 4 + i] = space::White;
    }

    // placed piece is central in the five-in-a-row
    EXPECT_TRUE(check_five_in_a_row(b, {4, 6}));
}

// Checking for five in a row at the edge of the board.
TEST(penteboard, check_five_in_a_row_edge_clamped)
{
    board b{};

    for (size_t x = 0; x < 5; ++x)
    {
        b[x, 0] = space::Black;
    }

    // placed piece is at the edge of the five-in-a-row
    EXPECT_TRUE(check_five_in_a_row(b, {0, 0}));
}

// Checking for five in a row when there are not enough consecutive pieces.
TEST(penteboard, check_five_in_a_row_not_enough)
{
    board b{};

    for (size_t y = 7; y < 11; ++y)
    {
        b[12, y] = space::White;
    }

    // placed piece is central in the four-in-a-row
    EXPECT_FALSE(check_five_in_a_row(b, {12, 9}));
}

