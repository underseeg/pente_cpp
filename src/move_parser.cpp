#include "move_parser.h"

#include <charconv>
#include <cctype>
#include <system_error>

namespace cj::pente::move_parser
{
    namespace
    {
        constexpr char toupper_safe(char c) noexcept
        {
            return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }

        struct parsed_move
        {
            coord position{0, 0};
            bool seenHorizontalDirection{false};
            bool seenVerticalDirection{false};
        };

        //TODO many code review changes

        std::expected<std::string_view, move_parse_error> parse_and_apply_component(
            parsed_move& move,
            std::string_view input)
        {
            if (input.empty())
            {
                return std::unexpected{move_parse_error::invalid_format};
            }

            // parse the letter part
            const auto direction = toupper_safe(input.front());
            input.remove_prefix(1);
            if (direction != 'U' && direction != 'D' && direction != 'L' && direction != 'R')
            {
                return std::unexpected{move_parse_error::invalid_format};
            }

            // parse the numeric part
            std::size_t distance{0};
            const auto parsed = std::from_chars(input.begin(), input.end(), distance);
            if (parsed.ec == std::errc::result_out_of_range)
            {
                return std::unexpected{move_parse_error::out_of_bounds};
            }
            else if (parsed.ec != std::errc{})
            {
                return std::unexpected{move_parse_error::invalid_format};
            }

            // handle repeated directions
            const bool horizontal = direction == 'L' || direction == 'R';
            const bool positive = direction == 'R' || direction == 'D';
            auto& seenDirection = horizontal ? move.seenHorizontalDirection : move.seenVerticalDirection;
            if (seenDirection)
            {
                return std::unexpected{move_parse_error::invalid_format};
            }

            // check if the move would go out of bounds
            auto& coordinate = horizontal ? move.position.x : move.position.y;
            if ((positive && distance >= GridSize - coordinate)
                || (!positive && distance > coordinate))
            {
                return std::unexpected{move_parse_error::out_of_bounds};
            }
            seenDirection = true;
            coordinate = positive ? coordinate + distance : coordinate - distance;
            return std::string_view{parsed.ptr, input.end()};
        }
    }

    std::expected<coord, move_parse_error> parse_move_from_string(std::string_view input)
    {
        if (input.size() == 1 && toupper_safe(input.front()) == 'O')
        {
            return coord{BoardCentre, BoardCentre};
        }

        parsed_move move{{BoardCentre, BoardCentre}};
        auto first = parse_and_apply_component(move, input);
        if (!first)
        {
            return std::unexpected{first.error()};
        }
        input = *first;

        // Parse the second component of the move if it exists.
        if (!input.empty())
        {
            auto second = parse_and_apply_component(move, input);
            if (!second)
            {
                return std::unexpected{second.error()};
            }
            input = *second;
            if (!input.empty())
            {
                return std::unexpected{move_parse_error::invalid_format};
            }
        }

        return move.position;
    }
}
