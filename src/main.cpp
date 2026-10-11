#include "pentegame.h"
#include "move_parser.h"

#include <string>
#include <random>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace cj::pente;

namespace
{
    piece pick_start_colour()
    {
        default_random_engine engine{random_device{}()};
        uniform_int_distribution<int> distribution{0, 1};
        return distribution(engine) == 0 ? piece::Black : piece::White;
    }

    char to_char(space value)
    {
        switch (value)
        {
            case space::Empty: return '.';
            case space::Black: return 'B';
            case space::White: return 'W';
        }
    
        throw logic_error{"Invalid space value"};
    }

    const char* to_string(piece value)
    {
        switch (value)
        {
            case piece::Black: return "Black";
            case piece::White: return "White";
        }
    
        throw logic_error{"Invalid piece value"};
    }

    void print_board(const board& gameBoard)
    {
        for (const auto row : gameBoard.rows())
        {
            for (const auto value : row)
            {
                cout << to_char(value) << ' ';
            }
            cout << endl;
        }
    }

    coord get_next_move_from_input()
    {
        while (true)
        {
            cout << "Enter move from centre (e.g. R3U2 or O): ";

            string input;
            if (!(cin >> input))
            {
                throw runtime_error{"Failed to read move input"};
            }

            cout << endl;

            const auto parseResult = move_parser::parse_move_from_string(input);

            if (parseResult)
            {
                return *parseResult;
            }

            if (parseResult.error() == move_parser::move_parse_error::out_of_bounds)
            {
                cout << "Move is outside the board. Try again." << endl;
                continue;
            }

            cout << "Invalid move format. Use directions and distances like R3U2, or O for centre." << endl;
        }
    }

    void print_outcome(const game_state& state)
    {
        const game_outcome& outcome = *state.outcome;
        if (!outcome.winner)
        {
            cout << "The game ends in a draw." << endl;
            return;
        }

        const auto winner = to_string(*outcome.winner);
        if (has_reason(outcome.reasons, win_reason::captures))
        {
            const auto capturedPieces = captured_pieces_for(state.captures, *outcome.winner);
            cout << winner << " wins by capturing at least five pairs! Total captured pieces: "
                 << capturedPieces << endl;
        }
        if (has_reason(outcome.reasons, win_reason::five_in_a_row))
        {
            cout << winner << " wins by getting 5 in a row!" << endl;
        }
    }

        void print_starting_colour(const piece startingColour)
    {
        cout << "Starting colour: " << to_string(startingColour) << endl;
    }

    void print_current_turn(const piece currentPlayer)
    {
        cout << "Current turn: " << to_string(currentPlayer) << endl;
    }

    void print_move_error(const move_error& error)
    {
        switch (error)
        {
            case move_error::out_of_bounds:
                cout << "Move is outside the board. Try again." << endl;
                break;
            case move_error::occupied:
                cout << "That space is already occupied. Try again." << endl;
                break;
            case move_error::game_finished:
                throw logic_error{"Cannot play a move after the game has finished"};
        }
    }

    void print_any_captured_pairs(const move_report& result, const game_state& state)
    {
        if (result.pairs_captured > 0)
        {
            const auto movingPlayer = state.current_player;
            const auto totalCapturedPieces = captured_pieces_for(state.captures, movingPlayer);
            cout << to_string(movingPlayer) << " captured " << result.pairs_captured
                 << " pair(s). Total pairs: " << (totalCapturedPieces / 2) << endl;
        }
    }

    void run_game()
    {
        const auto startingColour = pick_start_colour();
        game game{startingColour};
        const auto& state = game.state();

        print_starting_colour(startingColour);

        while (!state.outcome)
        {
            print_board(state.board);
            print_current_turn(state.current_player);

            const auto position = get_next_move_from_input();
            const auto result = game.play(position);
            if (!result)
                print_move_error(result.error());
            else
                print_any_captured_pairs(*result, state);
        }
        
        print_outcome(state);
        print_board(state.board);
    }
}

int main()
{
    try
    {
        run_game();
    }
    catch (const exception& ex)
    {
        cerr << "Fatal error: " << ex.what() << endl;
        return 1;
    }
    return 0;
}
