#include "penteboard.h"

#include <gtest/gtest.h>

#include <ranges>

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
        return x < y ? space::Black : space::White;
    }

    coord to_coord(size_t index)
    {
        return {index % GridSize, index / GridSize};
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

// In and out of bounds checks for board coordinates.
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
TEST(penteboard, rows_matches_indexing)
{
    // Arrange
    board b{};

    const auto coords = views::iota(0zu, GridSize * GridSize) | views::transform(to_coord);

    // Act
    for (const auto& position : coords)
    {
        b[position] = space_from_coords(position.x, position.y);
    }

    // Assert
    EXPECT_EQ(b.rows().size(), GridSize);

    for (const auto [y, row] : std::views::enumerate(b.rows()))
    {
        EXPECT_EQ(row.size(), GridSize);

        for (auto [x, value] : std::views::enumerate(row))
        {
            EXPECT_EQ(value, space_from_coords(x, y));
            EXPECT_EQ(value, (b[x, y]));
        }
    }
}
