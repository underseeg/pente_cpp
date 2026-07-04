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

        struct direction
        {
            ptrdiff_t dx;
            ptrdiff_t dy;
        };

        constexpr array<direction, 4> AxisDirections {{
            {1, 0}, // Horizontal axis
            {0, 1}, // Vertical axis
            {1, 1}, // Falling diagonal axis (top-left to bottom-right)
            {1, -1} // Rising diagonal axis (top-right to bottom-left)
        }};

        direction opposite(direction axisDirection)
        {
            return direction {
                .dx = -axisDirection.dx,
                .dy = -axisDirection.dy
            };
        }

        std::size_t max_steps_to_edge(std::size_t point, std::ptrdiff_t delta)
        {
            if (delta > 0)
                return (GridSize - 1) - point;
            else if (delta < 0)
                return point;
            else
                return (GridSize - 1);
        }

        std::size_t ray_steps_to_edge(std::size_t x, std::size_t y, direction rayDirection)
        {
            const auto maxX = max_steps_to_edge(x, rayDirection.dx);
            const auto maxY = max_steps_to_edge(y, rayDirection.dy);
            return min(maxX, maxY);
        }

        std::ptrdiff_t to_end_point(std::size_t start, std::ptrdiff_t step, std::ptrdiff_t length)
        {
            return static_cast<ptrdiff_t>(start) + ((length - 1) * step);
        }

        /// @brief Generates a range of board positions in a specified direction from a starting point.
        ///         Behaviour is undefined if (x, y) is out of bounds.
        /// @param boardView The view of the board.
        /// @param x The starting x-coordinate.
        /// @param y The starting y-coordinate.
        /// @param rayDirection The direction in which to generate the range.
        /// @return A range of board positions in the specified direction.
        auto ray(board& board, std::size_t x, std::size_t y, direction rayDirection)
        {
            const ptrdiff_t length = min(ray_steps_to_edge(x, y, rayDirection), MaxStepsPerDirection) + 1;

            return views::iota(0z, length)
                | views::transform([=, &board](ptrdiff_t step) -> space& {
                    const size_t rayX = static_cast<ptrdiff_t>(x) + (rayDirection.dx * step);
                    const size_t rayY = static_cast<ptrdiff_t>(y) + (rayDirection.dy * step);
                    return board[rayX, rayY];
                });
        }

        auto ray(const board& board, std::size_t x, std::size_t y, direction rayDirection)
        {
            const ptrdiff_t length = min(ray_steps_to_edge(x, y, rayDirection), MaxStepsPerDirection) + 1;

            return views::iota(0z, length)
                | views::transform([=, &board](ptrdiff_t step) -> const space& {
                    const size_t rayX = static_cast<ptrdiff_t>(x) + (rayDirection.dx * step);
                    const size_t rayY = static_cast<ptrdiff_t>(y) + (rayDirection.dy * step);
                    return board[rayX, rayY];
                });
        }

        int contiguous_matches_from_origin(const board& board, std::size_t x, std::size_t y, direction axisDirection)
        {
            const auto playerSpace = board[x, y];
            const auto contiguous_matches = [playerSpace](const auto& range)
            {
                return ranges::distance(
                    range
                    | views::take_while([playerSpace](space currentSpace) {
                        return currentSpace == playerSpace;
                    }));
            };

            return contiguous_matches(ray(board, x, y, axisDirection));
        }

        bool axis_has_five_in_a_row(const board& board, std::size_t x, std::size_t y, direction axisDirection)
        {
            const auto forwardCount = contiguous_matches_from_origin(board, x, y, axisDirection);
            const auto reverseCount = contiguous_matches_from_origin(board, x, y, opposite(axisDirection));

            return (forwardCount + reverseCount - 1) >= 5;
        }

        bool is_span_in_bounds(std::size_t startX, std::size_t startY, direction axisDirection, std::ptrdiff_t length)
        {
            const auto endX = to_end_point(startX, axisDirection.dx, length);
            const auto endY = to_end_point(startY, axisDirection.dy, length);
            return is_in_bounds(startX, startY) && is_in_bounds(endX, endY);
        }

        // behaviour is undefined if space is Empty
        space opposite(space space)
        {
            return (space == space::Black ? space::White : space::Black);
        }

        unsigned int capture_along_axis(board& board, std::size_t startX, std::size_t startY, direction axisDirection)
        {
            constexpr ptrdiff_t CaptureLength = 4;
            if (!is_span_in_bounds(startX, startY, axisDirection, CaptureLength))
            {
                return 0u;
            }

            auto line = ray(board, startX, startY, axisDirection);
            const auto playerSpace = board[startX, startY];
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
            && x < GridSize
            && y < GridSize;
    }

    unsigned int apply_captures(board& board, std::size_t x, std::size_t y)
    {
        unsigned int capturedPieces = 0;

        for (const auto& axisDirection : AxisDirections)
        {
            capturedPieces += capture_along_axis(board, x, y, axisDirection);
            capturedPieces += capture_along_axis(board, x, y, opposite(axisDirection));
        }

        return capturedPieces;
    }

    std::pair<unsigned int, bool> add_captured_pieces(capture_pots& pots, piece player, unsigned int piecesToAdd)
    {
        auto& totalCapturedPieces = player == piece::Black ? pots.pieces_captured_by_black : pots.pieces_captured_by_white;
        totalCapturedPieces += piecesToAdd;

        return {totalCapturedPieces, (totalCapturedPieces / 2) >= 5};
    }

    bool check_five_in_a_row(const board& board, std::size_t x, std::size_t y)
    {
        return ranges::any_of(AxisDirections, [&](const direction& axisDirection) {
            return axis_has_five_in_a_row(board, x, y, axisDirection);
        });
    }
}
