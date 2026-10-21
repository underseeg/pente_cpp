#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <ranges>
#include <utility>

/// @brief Namespace for the Pente game engine.
namespace cj::pente
{
    constexpr std::size_t GridSize {19}; ///< The size of the Pente game board, which is a 19x19 grid.
    constexpr auto BoardCentre {GridSize / 2}; ///< The center coordinate of the Pente game board, which is at (9, 9).

    /// @brief Represents a player piece, which can be either Black or White.
    /// @see space for a space on the board, which may also be Empty.
    enum class piece
    {
        Black,
        White
    };

    /// @brief Represents a space on the board, which can be Empty, Black, or White.
    /// @see piece for a player piece, which can be Black or White but not Empty.
    enum class space
    {
        Empty,
        Black,
        White
    };

    // forward declaration to enable explicit cast oeprator
    struct signed_coord;

    /// @brief Represents a coordinate on the board with unsigned integer values.
    struct coord
    {
        std::size_t x; ///< The x-coordinate of the space.
        std::size_t y; ///< The y-coordinate of the space.

        /// @brief Explicit conversion operator to convert coord to signed_coord.
        explicit constexpr operator signed_coord() const;
    };

    /// @brief Represents a coordinate on the board with signed integer values, mainly internal use.
    struct signed_coord
    {
        std::ptrdiff_t x; ///< The x-coordinate of the space.
        std::ptrdiff_t y; ///< The y-coordinate of the space.

        /// @brief Explicit conversion operator to convert signed_coord to coord.
        explicit constexpr operator coord() const;
    };

    constexpr coord::operator signed_coord() const
        {
            return signed_coord {static_cast<std::ptrdiff_t>(x), static_cast<std::ptrdiff_t>(y)};
        }

    constexpr signed_coord::operator coord() const
    {
        return coord {static_cast<std::size_t>(x), static_cast<std::size_t>(y)};
    }

    /// @brief Represents the game board, which is a 19x19 grid of spaces.
    ///
    /// Provides methods for accessing spaces and rows.
    class board
    {
    public:
        /// @brief Accesses the space at the given coordinates. Behaviour is undefined if (x, y) is out of bounds.
        /// @param x The x-coordinate of the space.
        /// @param y The y-coordinate of the space.
        /// @return A cv-qualified reference to the space at the given coordinates.
        auto&& operator[](this auto&& self, std::size_t x, std::size_t y) noexcept
        {
            return std::forward<decltype(self)>(self).data[y * GridSize + x];
        }

        /// @brief Accesses the space at the given coordinates. Behaviour is undefined if position is out of bounds.
        /// @param position The coordinates of the space.
        /// @return A cv-qualified reference to the space at the given coordinates.
        auto&& operator[](this auto&& self, coord position) noexcept
        {
            return std::forward<decltype(self)>(self).data[position.y * GridSize + position.x];
        }

        /// @brief Returns a view of the board rows. Each row is represented as a span of spaces.
        inline auto rows(this auto&& self) noexcept
        {
            auto* cells = self.data.data();
            return std::views::iota(0zu, GridSize)
                | std::views::transform([cells](std::size_t row) {
                    return std::span{cells + row * GridSize, GridSize};
                });
        }

        auto begin(this auto&& self) noexcept
        {
            return self.data.begin();
        }

        auto end(this auto&& self) noexcept
        {
            return self.data.end();
        }

    private:
        std::array<space, GridSize * GridSize> data{};
    };

    /// @brief Returns the opposite player piece.
    /// @param piece The player piece.
    /// @return The opposite player piece.
    constexpr piece opposite(piece piece)
    {
        return (piece == piece::Black ? piece::White : piece::Black);
    }

    /// @brief Converts a player piece to the corresponding space type.
    /// @param piece The player piece.
    /// @return The corresponding space type (Black or White).
    constexpr space as_space(piece piece)
    {
        return (piece == piece::Black ? space::Black : space::White);
    }

    /// @brief Checks if the given coordinates are within the bounds of the board.
    /// @param position The coordinates to check.
    /// @return True if the coordinates are within bounds, false otherwise.
    bool is_in_bounds(coord position);

    /// @brief Checks if the given coordinates are within the bounds of the board.
    /// @param position The coordinates to check.
    /// @return True if the coordinates are within bounds, false otherwise.
    bool is_in_bounds(signed_coord position);
}
