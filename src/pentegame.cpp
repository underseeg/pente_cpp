#include "pentegame.h"

#include <algorithm>
#include <array>
#include <ranges>
#include <utility>
#include <type_traits>

namespace cj::pente
{
    using namespace std;

    namespace
    {
        // The four axes are checked in both directions for captures and runs.
        constexpr array<signed_coord, 4> AxisDirections{{
            {1,  0}, // Horizontal axis
            {0,  1}, // Vertical axis
            {1,  1}, // Falling diagonal axis (top-left to bottom-right)
            {1, -1} // Rising diagonal axis (top-right to bottom-left)
        }};

        constexpr signed_coord operator+(signed_coord position, signed_coord delta) { return {position.x + delta.x, position.y + delta.y}; }
        constexpr signed_coord operator*(signed_coord position, ptrdiff_t scale) { return {position.x * scale, position.y * scale}; }
        constexpr signed_coord operator-(signed_coord position) { return {-position.x, -position.y}; }

        /// @brief Returns the distance to the board edge, including the starting point.
        /// A zero delta never reaches an edge, so GridSize is returned.
        size_t distance_to_edge(size_t point, ptrdiff_t delta)
        {
            if (delta > 0)
                return GridSize - point;
            if (delta < 0)
                return point + 1;
            else
                return GridSize;
        }

        size_t distance_to_edge(coord position, signed_coord direction)
        {
            return min(distance_to_edge(position.x, direction.x),
                       distance_to_edge(position.y, direction.y));
        }

        signed_coord advance(coord start, signed_coord step, ptrdiff_t length)
        {
            return static_cast<signed_coord>(start) + step * length;
        }

        // Keep ray usable with both const boards and the mutable board used by captures.
        template <typename T>
        concept board_type = same_as<remove_cvref_t<T>, board>;

        /// @brief Generates a range up to five board spaces from start in specified direction.
        /// Behaviour is undefined if start is out of bounds.
        auto ray(board_type auto& gameBoard, coord start, signed_coord direction)
        {
            constexpr size_t MaxRayLength{5};
            const size_t length = min(distance_to_edge(start, direction), MaxRayLength);

            // returns a space& or const space& depending on whether board is const or not
            return views::iota(0z, static_cast<ptrdiff_t>(length))
                | views::transform([=, &gameBoard](ptrdiff_t step) -> decltype(auto) {
                    const auto position = advance(start, direction, step);
                    return gameBoard[static_cast<coord>(position)]; // start and length are guaranteed to be in bounds
                });
        }

        /// @brief Counts matching spaces from origin until the first different space.
        auto contiguous_matches_from_origin(const board& gameBoard, coord origin, signed_coord direction)
        {
            const auto playerSpace = gameBoard[origin];
            return ranges::distance(
                ray(gameBoard, origin, direction)
                | views::take_while([playerSpace](space currentSpace) {
                    return currentSpace == playerSpace;
                }));
        }

        /// @brief Checks for five contiguous pieces through origin along an axis.
        bool axis_has_five_in_a_row(const board& gameBoard, coord origin, signed_coord direction)
        {
            const auto forwardCount = contiguous_matches_from_origin(gameBoard, origin, direction);
            const auto reverseCount = contiguous_matches_from_origin(gameBoard, origin, -direction);
            return forwardCount + reverseCount - 1 >= 5;
        }

        /// @brief Checks that both endpoints of a directional span are on the board.
        // Behaviour is undefined if the start coordinate is out of bounds.
        bool span_is_in_bounds(coord start, signed_coord direction, ptrdiff_t length)
        {
            return is_in_bounds(advance(start, direction, length - 1));
        }

        // Behaviour is undefined if value is Empty.
        space opposite(space value)
        {
            return value == space::Black ? space::White : space::Black;
        }

        /// @brief Removes a bracketed pair starting from the newly played piece.
        unsigned int capture_pairs_along_direction(board& gameBoard, coord start, signed_coord direction)
        {
            constexpr ptrdiff_t CaptureLength{4};
            if (!span_is_in_bounds(start, direction, CaptureLength))
            {
                return 0u;
            }

            auto line = ray(gameBoard, start, direction);
            const auto playerSpace = gameBoard[start];
            const auto opponentSpace = opposite(playerSpace);

            // The played piece is at index zero; check the two opponents and the bracketing piece.
            if (line[1] != opponentSpace || line[2] != opponentSpace || line[3] != playerSpace)
            {
                return 0u;
            }

            line[1] = space::Empty;
            line[2] = space::Empty;
            return 2u;
        }

        /// @brief Applies pair captures on the board based on the last move made at the given position.
        /// Behaviour is undefined if the space at the given position is Empty.
        /// @param board The board on which to apply pair captures.
        /// @param playedPosition The coordinates of the last move.
        /// @return The number of pieces captured as a result of the last move.
        unsigned int apply_pair_captures(board& gameBoard, coord playedPosition)
        {
            unsigned int capturedPieces{0};
            for (const auto& direction : AxisDirections)
            {
                capturedPieces += capture_pairs_along_direction(gameBoard, playedPosition, direction);
                capturedPieces += capture_pairs_along_direction(gameBoard, playedPosition, -direction);
            }
            return capturedPieces;
        }

        /// @brief Checks if the last move made at the given position results in five contiguous pieces of the same color in a row.
        /// @param gameBoard The board to check.
        /// @param playedPosition The coordinates of the last move.
        /// @return True if the last move results in five contiguous pieces of the same color, false otherwise.
        bool has_five_in_a_row(const board& gameBoard, coord playedPosition)
        {
            return ranges::any_of(AxisDirections, [&](const signed_coord& direction) {
                return axis_has_five_in_a_row(gameBoard, playedPosition, direction);
            });
        }

        bool is_full(const board& gameBoard)
        {
            // an obvious optimisation would be to keep track of the number of empty spaces and return whether spaces == 0.
            return !ranges::contains(gameBoard, space::Empty);
        }

        game_state opening_state(piece startingColour)
        {
            game_state initialState;
            initialState.game_board[BoardCentre, BoardCentre] = as_space(startingColour);
            initialState.current_player = opposite(startingColour);
            return initialState;
        }

        /// @brief Returns a player's captured-piece total while preserving the pot's constness.
        template <typename Captures>
            requires std::same_as<std::remove_cvref_t<Captures>, capture_pots>
        constexpr auto& captured_pieces_for(Captures& captures, piece player)
        {
            return player == piece::Black
                ? captures.pieces_captured_by_black
                : captures.pieces_captured_by_white;
        }

        auto apply_move_and_captures(board& board, piece player, coord playedPosition)
        {
            board[playedPosition] = as_space(player);
            return apply_pair_captures(board, playedPosition);
        }

        auto update_capture_pot_for_player(capture_pots& captures, piece player, unsigned int capturedPieces)
        {
            auto& totalCapturedPieces = captured_pieces_for(captures, player);
            totalCapturedPieces += capturedPieces;
            return totalCapturedPieces;
        }
        
        void update_game_outcome(game_state& state, unsigned int totalCapturedPieces, bool fiveInARow)
        {
            win_reason reasons = totalCapturedPieces >= 10
                ? win_reason::captures
                : win_reason::none;

            if (fiveInARow)
            {
                reasons |= win_reason::five_in_a_row;
            }

            if (reasons != win_reason::none)
            {
                state.outcome = game_outcome{state.current_player, reasons};
            }
            // A full board is a defensive draw case; ordinary game progression ends on a win.
            else if (is_full(state.game_board))
            {
                state.outcome = game_outcome{nullopt, win_reason::none};
            }
        }
    }

    game::game(piece startingColour) noexcept
        : game(opening_state(startingColour))
    {
    }

    game::game(const game_state& initialState) noexcept
        : state{initialState}
    {
    }

    std::expected<move_report, move_error> game::play(coord position) noexcept
    {
        if (state.outcome)
        {
            return unexpected{move_error::game_finished};
        }
        if (!is_in_bounds(position))
        {
            return unexpected{move_error::out_of_bounds};
        }
        if (state.game_board[position] != space::Empty)
        {
            return unexpected{move_error::occupied};
        }

        const auto capturedPieces = apply_move_and_captures(state.game_board, state.current_player, position);
        auto totalCapturedPieces = update_capture_pot_for_player(state.captures, state.current_player, capturedPieces);
        update_game_outcome(state, totalCapturedPieces, has_five_in_a_row(state.game_board, position));
        
        // Switch to the next player if the game is not finished
        if (!state.outcome)
        {
            state.current_player = opposite(state.current_player);
        }

        return move_report{capturedPieces / 2};
    }

    const cj::pente::board& game::board() const noexcept
    {
        return state.game_board;
    }

    piece game::current_player() const noexcept
    {
        return state.current_player;
    }

    std::optional<game_outcome> game::outcome() const noexcept
    {
        return state.outcome;
    }

    unsigned int game::captures_by(piece player) const noexcept
    {
        return captured_pieces_for(state.captures, player);
    }
}
