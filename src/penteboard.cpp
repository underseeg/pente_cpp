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

        struct signed_coord
        {
            ptrdiff_t x;
            ptrdiff_t y;
        };

        struct offset
        {
            ptrdiff_t dx;
            ptrdiff_t dy;
        };

        constexpr array<offset, 4> AxisDirections {{
            {1,  0}, // Horizontal axis
            {0,  1}, // Vertical axis
            {1,  1}, // Falling diagonal axis (top-left to bottom-right)
            {1, -1} // Rising diagonal axis (top-right to bottom-left)
        }};

        constexpr signed_coord to_signed(coord position)
        {
            return signed_coord {
                .x = static_cast<ptrdiff_t>(position.x),
                .y = static_cast<ptrdiff_t>(position.y)
            };
        }

        constexpr coord to_unsigned(signed_coord position)
        {
            return coord {
                .x = static_cast<std::size_t>(position.x),
                .y = static_cast<std::size_t>(position.y)
            };
        }

        constexpr signed_coord operator+(coord position, offset delta)
        {
            return signed_coord {
                .x = static_cast<ptrdiff_t>(position.x) + delta.dx,
                .y = static_cast<ptrdiff_t>(position.y) + delta.dy
            };
        }

        constexpr signed_coord operator+(signed_coord position, offset delta)
        {
            return signed_coord {
                .x = position.x + delta.dx,
                .y = position.y + delta.dy
            };
        }

        constexpr offset operator*(offset delta, ptrdiff_t scale)
        {
            return offset {
                .dx = delta.dx * scale,
                .dy = delta.dy * scale
            };
        }

        constexpr offset opposite(offset axisOffset)
        {
            return offset {
                .dx = -axisOffset.dx,
                .dy = -axisOffset.dy
            };
        }

        constexpr offset operator*(offset delta, std::size_t scale)
        {
            return delta * static_cast<ptrdiff_t>(scale);
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

        std::size_t ray_steps_to_edge(coord position, offset rayDirection)
        {
            const auto maxX = max_steps_to_edge(position.x, rayDirection.dx);
            const auto maxY = max_steps_to_edge(position.y, rayDirection.dy);
            return min(maxX, maxY);
        }

        signed_coord to_end_point(coord start, offset step, std::ptrdiff_t length)
        {
            return start + (step * (length - 1));
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
        auto ray(board_type auto& board, coord start, offset rayDirection)
        {
            const size_t length = min(ray_steps_to_edge(start, rayDirection), MaxStepsPerDirection) + 1u;

            // returns a space& or const space& depending on whether board is const or not
            return views::iota(0zu, length)
                | views::transform([=, &board](size_t step) -> decltype(auto) {
                    const auto position = start + (rayDirection * step);
                    return board[to_unsigned(position)]; // start and length are guaranteed to be in bounds
                });
        }

        auto contiguous_matches_from_origin(const board& board, coord origin, offset axisDirection)
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

        bool axis_has_five_in_a_row(const board& board, coord origin, offset axisDirection)
        {
            const auto forwardCount = contiguous_matches_from_origin(board, origin, axisDirection);
            const auto reverseCount = contiguous_matches_from_origin(board, origin, opposite(axisDirection));

            return (forwardCount + reverseCount - 1) >= 5;
        }

        bool is_span_in_bounds(coord start, offset axisDirection, std::ptrdiff_t length)
        {
            const auto end = to_end_point(start, axisDirection, length);
            return start.x < GridSize && start.y < GridSize
                && is_in_bounds(end.x, end.y);
        }

        // behaviour is undefined if space is Empty
        space opposite(space space)
        {
            return (space == space::Black ? space::White : space::Black);
        }

        unsigned int capture_along_axis(board& board, coord start, offset axisDirection)
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

    unsigned int apply_captures(board& board, coord playedPosition)
    {
        unsigned int capturedPieces = 0;

        for (const auto& axisDirection : AxisDirections)
        {
            capturedPieces += capture_along_axis(board, playedPosition, axisDirection);
            capturedPieces += capture_along_axis(board, playedPosition, opposite(axisDirection));
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
        return ranges::any_of(AxisDirections, [&](const offset& axisDirection) {
            return axis_has_five_in_a_row(board, playedPosition, axisDirection);
        });
    }
}
