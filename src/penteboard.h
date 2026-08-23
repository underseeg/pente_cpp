#pragma once

#include <array>
#include <span>
#include <ranges>
#include <utility>

namespace cj::pente
{
    constexpr std::size_t GridSize {19};
    constexpr auto BoardCentre {GridSize / 2};

    enum class piece
    {
        Black,
        White
    };

    enum class space
    {
        Empty,
        Black,
        White
    };

    // forward declaration to enable explicit cast oeprator
    struct signed_coord;

    struct coord
    {
        std::size_t x;
        std::size_t y;

        explicit constexpr operator signed_coord() const;
    };

    struct signed_coord
    {
        std::ptrdiff_t x;
        std::ptrdiff_t y;

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

    class board
    {
    public:
        /// @brief Accesses the space at the given coordinates. Behaviour is undefined if (x, y) is out of bounds.
        /// @param x The x-coordinate of the space.
        /// @param y The y-coordinate of the space.
        /// @return A cv-qualified reference to the space at the given coordinates.
        auto&& operator[](this auto&& self, std::size_t x, std::size_t y)
        {
            return std::forward<decltype(self)>(self).data[y * GridSize + x];
        }

        /// @brief Accesses the space at the given coordinates. Behaviour is undefined if position is out of bounds.
        /// @param position The coordinates of the space.
        /// @return A cv-qualified reference to the space at the given coordinates.
        auto&& operator[](this auto&& self, coord position)
        {
            return std::forward<decltype(self)>(self).data[position.y * GridSize + position.x];
        }

        /// @brief Returns a view of the rows of the board, where each row is a span of type: const space*.
        /// @return A view of the rows of the board.
        inline auto rows() const
        {
            return std::views::iota(0zu, GridSize)
                | std::views::transform([this](std::size_t row) {
                    return std::span{data.data() + row * GridSize, GridSize};
                });
        }

    private:
        std::array<space, GridSize * GridSize> data{};
    };

    struct capture_pots
    {
        unsigned int pieces_captured_by_black {0};
        unsigned int pieces_captured_by_white {0};
    };

    constexpr piece opposite(piece piece)
    {
        return (piece == piece::Black ? piece::White : piece::Black);
    }

    constexpr space to_space(piece piece)
    {
        return (piece == piece::Black ? space::Black : space::White);
    }

    /// @brief Checks if the given coordinates are within the bounds of the board.
    /// @param position The coordinates to check.
    /// @return True if the coordinates are within bounds, false otherwise.
    bool is_in_bounds(signed_coord position);
    bool is_in_bounds(coord position);

    /// @brief Applies pair captures on the board based on the last move made at the given position.
    /// Behaviour is undefined if the space at the given position is Empty.
    /// @param board The board on which to apply pair captures.
    /// @param playedPosition The coordinates of the last move.
    /// @return The number of pieces captured as a result of the last move.
    unsigned int apply_pair_captures(board& board, coord playedPosition);

    /// @brief Adds captured pieces to the player's capture pot and checks for a win by captures.
    /// @param pots The capture pots to update.
    /// @param player The player who captured the pieces.
    /// @param piecesToAdd The number of pieces to add to the player's capture pot.
    /// @return A pair containing the total captured pieces and a boolean indicating if the player has won by captures.
    std::pair<unsigned int, bool> add_captured_pieces(capture_pots& pots, piece player, unsigned int piecesToAdd);

    /// @brief Checks if the last move made at the given position results in five contiguous pieces of the same color in a row.
    /// @param board The board to check.
    /// @param playedPosition The coordinates of the last move.
    /// @return True if the last move results in five contiguous pieces of the same color, false otherwise.
    bool check_five_in_a_row(const board& board, coord playedPosition);
}
