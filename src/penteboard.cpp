#include "penteboard.h"

#include <algorithm>
#include <array>
#include <mdspan>
#include <span>
#include <ranges>

namespace cj::pente
{
    using namespace std;

    namespace
    {
        /// @brief Maximum number of steps to take in a single direction when checking for contiguous pieces or captures.
        constexpr size_t MaxStepsPerDirection {4};

        constexpr array<signed_coord, 4> AxisDirections {{
            {1,  0}, // Horizontal axis
            {0,  1}, // Vertical axis
            {1,  1}, // Falling diagonal axis (top-left to bottom-right)
            {1, -1} // Rising diagonal axis (top-right to bottom-left)
        }};

        constexpr signed_coord operator+(signed_coord coord, signed_coord delta) { return signed_coord {coord.x + delta.x, coord.y + delta.y}; }
        constexpr signed_coord operator*(signed_coord coord, ptrdiff_t scale) { return signed_coord {coord.x * scale, coord.y * scale}; }
        constexpr signed_coord operator-(signed_coord coord) { return signed_coord {-coord.x, -coord.y}; }

        std::size_t max_steps_to_edge(std::size_t point, std::ptrdiff_t delta)
        {
            if (delta > 0)
                return (GridSize - 1) - point;
            else if (delta < 0)
                return point;
            else
                return (GridSize - 1);
        }

        std::size_t ray_steps_to_edge(coord position, signed_coord rayDirection)
        {
            const auto maxX = max_steps_to_edge(position.x, rayDirection.x);
            const auto maxY = max_steps_to_edge(position.y, rayDirection.y);
            return min(maxX, maxY);
        }

        signed_coord advance(coord start, signed_coord step, std::ptrdiff_t length)
        {
            return static_cast<signed_coord>(start) + step * length;
        }

        // define a concept for the board type so ray can handle both const and non-const boards
        template <typename T>
        concept board_type = std::same_as<std::remove_cvref_t<T>, board>;

        /// @brief Generates a range of board positions in a specified direction from a starting point.
        ///         Behaviour is undefined if (x, y) is out of bounds.
        /// @param board The board.
        /// @param x The starting x-coordinate.
        /// @param y The starting y-coordinate.
        /// @param rayDirection The direction in which to generate the range.
        /// @return A range of board positions in the specified direction.
        auto ray(board_type auto& board, coord start, signed_coord rayDirection)
        {
            const size_t length = min(ray_steps_to_edge(start, rayDirection), MaxStepsPerDirection) + 1u;

            // returns a space& or const space& depending on whether board is const or not
            return views::iota(0zu, length)
                | views::transform([=, &board](ptrdiff_t step) -> decltype(auto) {
                    const auto position = advance(start, rayDirection, step);
                    return board[static_cast<coord>(position)]; // start and length are guaranteed to be in bounds
                });
        }

        auto contiguous_matches_from_origin(const board& board, coord origin, signed_coord axisDirection)
        {
            const auto playerSpace = board[origin];
            const auto contiguous_matches = [playerSpace](const auto& range)
            {
                return ranges::distance(
                    range
                    | views::take_while([playerSpace](space currentSpace) {
                        return currentSpace == playerSpace;
                    }));
            };

            return contiguous_matches(ray(board, origin, axisDirection));
        }

        bool axis_has_five_in_a_row(const board& board, coord origin, signed_coord axisDirection)
        {
            const auto forwardCount = contiguous_matches_from_origin(board, origin, axisDirection);
            const auto reverseCount = contiguous_matches_from_origin(board, origin, -axisDirection);

            return (forwardCount + reverseCount - 1) >= 5;
        }

        bool is_span_in_bounds(coord start, signed_coord axisDirection, std::ptrdiff_t length)
        {
            const auto last = advance(start, axisDirection, length - 1);
            return start.x < GridSize && start.y < GridSize
                && is_in_bounds(last.x, last.y);
        }

        // behaviour is undefined if space is Empty
        space opposite(space space)
        {
            return (space == space::Black ? space::White : space::Black);
        }

        unsigned int capture_pairs_along_direction(board& board, coord start, signed_coord axisDirection)
        {
            constexpr ptrdiff_t CaptureLength = 4;
            if (!is_span_in_bounds(start, axisDirection, CaptureLength))
            {
                return 0u;
            }

            auto line = ray(board, start, axisDirection);
            const auto playerSpace = board[start];
            const auto opponentSpace = opposite(playerSpace);

            if (line[0] != playerSpace || line[1] != opponentSpace || line[2] != opponentSpace || line[3] != playerSpace)
            {
                return 0u;
            }

            line[1] = space::Empty;
            line[2] = space::Empty;
            return 2u;
        }
    }

    bool is_in_bounds(std::ptrdiff_t x, std::ptrdiff_t y)
    {
        return x >= 0 && y >= 0
            && x < static_cast<std::ptrdiff_t>(GridSize)
            && y < static_cast<std::ptrdiff_t>(GridSize);
    }

    unsigned int apply_pair_captures(board& board, coord playedPosition)
    {
        unsigned int capturedPieces = 0;

        for (const auto& axisDirection : AxisDirections)
        {
            capturedPieces += capture_pairs_along_direction(board, playedPosition, axisDirection);
            capturedPieces += capture_pairs_along_direction(board, playedPosition, -axisDirection);
        }

        return capturedPieces;
    }

    std::pair<unsigned int, bool> add_captured_pieces(capture_pots& pots, piece player, unsigned int piecesToAdd)
    {
        auto& totalCapturedPieces = player == piece::Black ? pots.pieces_captured_by_black : pots.pieces_captured_by_white;
        totalCapturedPieces += piecesToAdd;

        return {totalCapturedPieces, (totalCapturedPieces / 2) >= 5};
    }

    bool check_five_in_a_row(const board& board, coord playedPosition)
    {
        return ranges::any_of(AxisDirections, [&](const signed_coord& axisDirection) {
            return axis_has_five_in_a_row(board, playedPosition, axisDirection);
        });
    }
}
