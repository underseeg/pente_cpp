#pragma once

#include <array>
#include <mdspan>
#include <span>
#include <ranges>

namespace cj::pente
{
    constexpr std::size_t GridSize {19};
    constexpr auto BoardCentre = GridSize / 2;

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

    using board_array = std::array<space, GridSize * GridSize>;
    using board_view = std::mdspan<space, std::extents<std::size_t, GridSize, GridSize>>;

    class board
    {
    public:
        board() = default;
        // could implement but don't need
        board(const board& other) = delete;
        board(board&& other) noexcept = delete;
        board& operator=(const board& other) = delete;
        board& operator=(board&& other) noexcept = delete;

        /// @brief Accesses the space at the given coordinates. Behaviour is undefined if (x, y) is out of bounds.
        /// @param x The x-coordinate of the space.
        /// @param y The y-coordinate of the space.
        /// @return A reference to the space at the given coordinates.
        auto operator[](std::size_t x, std::size_t y) -> space&
        {
            return view[y, x];
        }

        auto operator[](std::size_t x, std::size_t y) const -> const space&
        {
            return view[y, x];
        }

        /// @brief Returns a view of the rows of the board, where each row is a span of type: space.
        /// @return A view of the rows of the board.
        inline auto rows() const
        {
            return std::views::iota(0zu, GridSize)
                | std::views::transform([this](std::size_t row) {
                    return std::span{view.data_handle() + row * GridSize, GridSize};
                });
        }

    private:
        board_array data{};
        const board_view view{data.data()};
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
    /// @param x The x-coordinate to check.
    /// @param y The y-coordinate to check.
    /// @return True if the coordinates are within bounds, false otherwise.
    bool is_in_bounds(std::ptrdiff_t x, std::ptrdiff_t y);

    /// @brief Applies captures on the board based on the last move made at (x, y).
    /// Behaviour is undefined if the space at (x, y) is Empty.
    /// @param board The board on which to apply captures.
    /// @param x The x-coordinate of the last move.
    /// @param y The y-coordinate of the last move.
    /// @return The number of pieces captured as a result of the last move.
    unsigned int apply_captures(board& board, std::size_t x, std::size_t y);

    /// @brief Adds captured pieces to the player's capture pot and checks for a win by captures.
    /// @param pots The capture pots to update.
    /// @param player The player who captured the pieces.
    /// @param piecesToAdd The number of pieces to add to the player's capture pot.
    /// @return A pair containing the total captured pieces and a boolean indicating if the player has won by captures.
    std::pair<unsigned int, bool> add_captured_pieces(capture_pots& pots, piece player, unsigned int piecesToAdd);

    /// @brief Checks if the last move made at (x, y) results in five contiguous pieces of the same color in a row.
    /// @param board The board to check.
    /// @param x The x-coordinate of the last move.
    /// @param y The y-coordinate of the last move.
    /// @return True if the last move results in five contiguous pieces of the same color, false otherwise.
    bool check_five_in_a_row(const board& board, std::size_t x, std::size_t y);
}
