#include "move_parser.h"

#include <charconv>
#include <cctype>
#include <system_error>

namespace cj::pente::move_parser
{
    namespace
    {
        struct parsed_move
        {
            coord position{BoardCentre, BoardCentre};
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

            const auto direction = std::toupper(static_cast<unsigned char>(input.front()));
            if (direction != 'U' && direction != 'D' && direction != 'L' && direction != 'R')
            {
                return std::unexpected{move_parse_error::invalid_format};
            }

            std::size_t distance{0};
            const auto* number = input.data() + 1;
            const auto parsed = std::from_chars(number, input.data() + input.size(), distance);
            if (parsed.ec == std::errc::result_out_of_range)
            {
                return std::unexpected{move_parse_error::out_of_bounds};
            }
            if (parsed.ec != std::errc{})
            {
                return std::unexpected{move_parse_error::invalid_format};
            }

            const bool horizontal = direction == 'L' || direction == 'R';
            const bool positive = direction == 'R' || direction == 'D';
            auto& seenDirection = horizontal ? move.seenHorizontalDirection : move.seenVerticalDirection;
            if (seenDirection)
            {
                return std::unexpected{move_parse_error::invalid_format};
            }

            auto& coordinate = horizontal ? move.position.x : move.position.y;
            if ((positive && distance > GridSize - 1 - coordinate)
                || (!positive && distance > coordinate))
            {
                return std::unexpected{move_parse_error::out_of_bounds};
            }
            seenDirection = true;
            coordinate = positive ? coordinate + distance : coordinate - distance;
            return input.substr(static_cast<std::size_t>(parsed.ptr - input.data()));
        }
    }

    std::expected<coord, move_parse_error> parse_move_from_string(std::string_view input)
    {
        if (input.size() == 1 && std::toupper(static_cast<unsigned char>(input.front())) == 'O')
        {
            return coord{BoardCentre, BoardCentre};
        }

        parsed_move move;
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
