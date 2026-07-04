#include "penteboard.h"

#include <expected>
#include <string>
#include <random>
#include <iostream>

using namespace std;
using namespace cj::pente;

template <typename RandomEngine>
piece pick_start_colour(RandomEngine& engine)
{
    std::uniform_int_distribution<int> dist{0, 1};
    return dist(engine) == 0 ? piece::Black : piece::White;
}

char to_char(space space)
{
    switch (space)
    {
        case space::Empty: return '.';
        case space::Black: return 'B';
        case space::White: return 'W';
    }
    
    throw logic_error{"Invalid space value"};
}

const char* to_string(piece piece)
{
    switch (piece)
    {
        case piece::Black: return "Black";
        case piece::White: return "White";
    }
    
    throw logic_error{"Invalid piece value"};
}

void print_board(const board& board)
{
    for (auto row_view : get_rows(board))
    {
        for (auto space : row_view)
        {
            cout << to_char(space) << ' ';
        }
        cout << endl;
    }
}

namespace cj::pente::move_parser
{
    enum class MoveParseError
    {
        InvalidFormat,
        OutOfBounds
    };

    // parses up down left right and distance from centre - "R3U2" is 3 spaces to the right and 2 spaces up from the
    // centre of the board. "O" is centre of the board
    // invalid parse results are returned as an unexpected value with a MoveParseError enum
    std::expected<std::pair<std::ptrdiff_t, std::ptrdiff_t>, MoveParseError> parse_move_from_string(const std::string& input)
    {
        if (input.size() == 1 && toupper(static_cast<unsigned char>(input[0])) == 'O')
        {
            return pair{BoardCentre, BoardCentre};
        }

        auto x = BoardCentre;
        auto y = BoardCentre;

        bool seenHorizontalDirection = false;
        bool seenVerticalDirection = false;
        bool hasComponent = false;

        std::ptrdiff_t i = 0;
        while (i < std::ssize(input))
        {
            const auto direction = static_cast<char>(toupper(static_cast<unsigned char>(input.cbegin()[i])));
            ++i;

            if (direction != 'U' && direction != 'D' && direction != 'L' && direction != 'R')
            {
                return unexpected{MoveParseError::InvalidFormat};
            }

            if (i >= std::ssize(input) || !isdigit(static_cast<unsigned char>(input.cbegin()[i])))
            {
                return unexpected{MoveParseError::InvalidFormat};
            }

            std::ptrdiff_t distance = 0;
            while (i < std::ssize(input) && isdigit(static_cast<unsigned char>(input.cbegin()[i])))
            {
                distance = (distance * 10) + static_cast<std::ptrdiff_t>(input.cbegin()[i] - '0');
                ++i;
            }

            if (direction == 'L' || direction == 'R')
            {
                if (seenHorizontalDirection)
                {
                    return unexpected{MoveParseError::InvalidFormat};
                }

                seenHorizontalDirection = true;
                x += (direction == 'R' ? distance : -distance);
            }
            else
            {
                if (seenVerticalDirection)
                {
                    return unexpected{MoveParseError::InvalidFormat};
                }

                seenVerticalDirection = true;
                y += (direction == 'D' ? distance : -distance);
            }

            hasComponent = true;
        }

        if (!hasComponent)
        {
            return unexpected{MoveParseError::InvalidFormat};
        }

        if (!is_in_bounds(x, y))
        {
            return unexpected{MoveParseError::OutOfBounds};
        }

        return pair{x, y};
    }
}

std::pair<std::ptrdiff_t, std::ptrdiff_t> next_move_from_input()
{
    while (true)
    {
        cout << "Enter move from centre (e.g. R3U2 or O): ";

        string input;
        if (!(cin >> input))
        {
            throw runtime_error{"Failed to read move input"};
        }

        const auto parseResult = move_parser::parse_move_from_string(input);

        if (parseResult.has_value())
        {
            return parseResult.value();
        }

        if (parseResult.error() == move_parser::MoveParseError::OutOfBounds)
        {
            cout << "Move is outside the board. Try again." << endl;
            continue;
        }

        cout << "Invalid move format. Use directions and distances like R3U2, or O for centre." << endl;
    }
}

int main()
{
    board board{};
    
    default_random_engine engine{random_device{}()};
    auto currentPlayer = pick_start_colour(engine);
    capture_pots capturePots{};

    cout << "Starting colour: " << to_string(currentPlayer) << endl;

    // place first piece in the centre of the board
    board[BoardCentre, BoardCentre] = to_space(currentPlayer);
    currentPlayer = opposite(currentPlayer);

    while (true)
    {
        print_board(board);
        cout << "Current turn: " << to_string(currentPlayer) << endl;

        // accept input for next move
        auto [x, y] = next_move_from_input();

        // retry if the space is already occupied
        if (board[x, y] != space::Empty)
        {
            cout << "That space is already occupied. Try again." << endl;
            continue;
        }

        // place piece on board
        board[x, y] = to_space(currentPlayer);

        // check for captures
        const auto capturedPieces = apply_captures(board, x, y);
        const auto [totalCapturedPieces, hasWonByCaptures] = add_captured_pieces(capturePots, currentPlayer, capturedPieces);

        if (capturedPieces > 0)
        {
            cout << to_string(currentPlayer) << " captured " << (capturedPieces / 2)
                 << " pair(s). Total pairs: " << (totalCapturedPieces / 2) << endl;

            if (hasWonByCaptures)
            {
                cout << to_string(currentPlayer) << " wins by capturing >=5 pairs! Total captured pieces: " << totalCapturedPieces << endl;
                break;
            }
        }

        // check for 5 in a row
        bool hasFiveInARow = check_five_in_a_row(board, x, y);
        if (hasFiveInARow)
        {
            cout << to_string(currentPlayer) << " wins by getting 5 in a row!" << endl;
            break;
        }

        // switch turns
        currentPlayer = opposite(currentPlayer);
    }

    // print final board state
    print_board(board);

    return 0;
}
