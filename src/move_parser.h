#pragma once

#include "penteboard.h"

#include <expected>
#include <string_view>

namespace cj::pente::move_parser
{
    /// @brief Why a move string could not be parsed.
    enum class move_parse_error
    {
        invalid_format,
        out_of_bounds
    };

    /// @brief Parses a move relative to the centre of the board.
    ///
    /// Directions are U, D, L, and R followed by a distance, for example R3U2.
    /// Each axis may be specified at most once. O selects the centre.
    /// Invalid input returns an error.
    std::expected<coord, move_parse_error> from_string(std::string_view input);
}
