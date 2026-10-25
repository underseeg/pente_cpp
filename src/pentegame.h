#pragma once

#include "penteboard.h"

#include <compare>
#include <expected>
#include <limits>
#include <optional>

namespace cj::pente
{
    /// @brief The captured-piece totals for both players.
    struct capture_pots
    {
        unsigned int pieces_captured_by_black {0}; ///< Pieces captured by the black player.
        unsigned int pieces_captured_by_white {0}; ///< Pieces captured by the white player.
    };

    /// @brief The win conditions completed by the final move.
    enum class win_reason : unsigned char
    {
        none = 0x0,
        captures = 0x1,
        five_in_a_row = 0x2
    };

    /// @brief Combines win conditions.
    constexpr win_reason& operator|=(win_reason& left, win_reason right)
    {
        left = static_cast<win_reason>(
            static_cast<unsigned char>(left) | static_cast<unsigned char>(right));
        return left;
    }

    /// @brief Checks whether a win condition is present.
    constexpr bool has_reason(win_reason reasons, win_reason reason)
    {
        return (static_cast<unsigned char>(reasons) & static_cast<unsigned char>(reason)) != 0;
    }

    /// @brief The winner and all win conditions completed by the final move.
    ///
    /// A missing winner represents a draw; draws have no win reasons.
    struct game_outcome
    {
        std::optional<piece> winner; ///< The winner of the game, if any.
        win_reason reasons{win_reason::none}; ///< The win reasons for the final move, if any.
    };

    /// @brief The observable state of a Pente game.
    struct game_state
    {
        board game_board; ///< Current board.
        capture_pots captures; ///< Captured-piece totals.
        piece current_player{piece::Black}; ///< Player whose turn it is, or the winner after a win.
        std::optional<game_outcome> outcome; ///< Empty until the game ends.
    };

    /// @brief A strongly typed count of captured pairs.
    class pair_count
    {
    public:
        constexpr explicit pair_count(unsigned int value) noexcept : count{value} { }
        constexpr unsigned int value() const noexcept { return count; }
        constexpr auto operator<=>(const pair_count&) const noexcept = default;

    private:
        unsigned int count = 0;
    };

    /// @brief Creates a pair_count from a pair-count integer literal.
    consteval pair_count operator""_pairs(unsigned long long value)
    {
        // A real codebase should test this limit and test compilation failure for the out-of-range case.
        if (value > std::numeric_limits<unsigned int>::max())
        {
            throw "pair count literal is out of range";
        }
        return pair_count{static_cast<unsigned int>(value)};
    }

    /// @brief Why a move was rejected.
    enum class move_error
    {
        out_of_bounds,
        occupied,
        game_finished
    };

    /// @brief Owns a Pente game's current board, capture totals, turn, and outcome.
    ///
    /// Construction with a starting colour places it at the board centre and makes
    /// the opposite colour the first player to move. Construction with a game_state
    /// adopts that state as-is. Invalid moves leave the state unchanged. A winning
    /// player keeps the final turn; a non-winning move advances to the other player.
    class game
    {
    public:
        /// @brief Starts a game with the chosen colour at the centre.
        /// The opposite colour is first to make a move.
        explicit game(piece startingColour) noexcept;

        /// @brief Starts from the supplied state without applying opening-game setup.
        explicit game(const game_state& initialState) noexcept;

        /// @brief Applies a legal move and updates captures, outcome, and turn.
        /// @return The number of pairs captured, or the reason the move was rejected.
        std::expected<pair_count, move_error> play(coord position) noexcept;

        /// @brief Returns a read-only reference to the current board.
        ///
        /// The reference remains valid for the lifetime of this game and reflects
        /// later calls to play().
        const cj::pente::board& board() const noexcept;

        /// @brief Returns a copy of the current player.
        piece current_player() const noexcept;

        /// @brief Returns a copy of the game's outcome, if finished.
        std::optional<game_outcome> outcome() const noexcept;

        /// @brief Returns a copy of the player's captured-piece total.
        unsigned int captures_by(piece player) const noexcept;

    private:
        // The game is the only mutable owner; callers observe this state by const reference.
        game_state state{};
    };
}
