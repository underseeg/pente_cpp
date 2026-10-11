#include "move_parser.h"

#include <charconv>
#include <functional>

namespace cj::pente::move_parser
{
    namespace
    {
        constexpr char toupper_safe(char c) noexcept
        {
            return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }

        struct parsed_segment
        {
            char direction{};
            std::size_t distance{};
            std::string_view remaining;
        };

        struct parsed_move
        {
            coord position{0, 0};
            bool horizontalDirectionApplied{false};
            bool verticalDirectionApplied{false};
        };

        std::expected<parsed_segment, move_parse_error> parse_segment(std::string_view input)
        {
            if (input.empty())
            {
                return std::unexpected{move_parse_error::invalid_format};
            }

            char direction = toupper_safe(input.front());
            if (direction != 'U' && direction != 'D' && direction != 'L' && direction != 'R')
            {
                return std::unexpected{move_parse_error::invalid_format};
            }

            input.remove_prefix(1);

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

            return parsed_segment{direction, distance, {parsed.ptr, input.end()}};
        }

        std::expected<std::string_view, move_parse_error> apply_segment(parsed_move& move, const parsed_segment& segment)
        {
            const bool horizontal = segment.direction == 'L' || segment.direction == 'R';
            const bool positive = segment.direction == 'R' || segment.direction == 'D';

            // We can only apply a direction once
            auto& directionApplied = horizontal ? move.horizontalDirectionApplied : move.verticalDirectionApplied;
            if (directionApplied)
            {
                return std::unexpected{move_parse_error::invalid_format};
            }

            // Confirm that the move stays within the grid boundaries
            auto& coordinate = horizontal ? move.position.x : move.position.y;
            if ((positive && segment.distance >= GridSize - coordinate)
                || (!positive && segment.distance > coordinate))
            {
                return std::unexpected{move_parse_error::out_of_bounds};
            }

            directionApplied = true;
            coordinate += positive ? segment.distance : -segment.distance;
            return {segment.remaining};
        }
    }

    std::expected<coord, move_parse_error> parse_move_from_string(std::string_view input)
    {
        using namespace std::placeholders;
        parsed_move move{{BoardCentre, BoardCentre}};

        if (input.size() == 1 && toupper_safe(input.front()) == 'O')
        {
            return move.position;
        }

        auto apply_to_move = std::bind(apply_segment, std::ref(move), _1);
        auto after_first = parse_segment(input).and_then(apply_to_move);
        if (!after_first)
        {
            return std::unexpected{after_first.error()};
        }
        input = *after_first;

        // Parse the second component of the move if it exists.
        if (!input.empty())
        {
            auto after_second = parse_segment(input).and_then(apply_to_move);
            if (!after_second)
            {
                return std::unexpected{after_second.error()};
            }
            input = *after_second;
            if (!input.empty())
            {
                return std::unexpected{move_parse_error::invalid_format};
            }
        }

        return move.position;
    }
}
