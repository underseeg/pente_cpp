#include "pentegame.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <initializer_list>
#include <utility>

using namespace cj::pente;
using namespace std;

namespace
{
    game_state make_game_state(
        piece currentPlayer,
        initializer_list<pair<coord, space>> initialPieces,
        capture_pots captures = {})
    {
        game_state initialState;
        initialState.current_player = currentPlayer;
        initialState.captures = captures;
        for (const auto& [position, value] : initialPieces)
        {
            initialState.board[position] = value;
        }
        return initialState;
    }
}

// The starting colour occupies the centre, and its opponent moves first.
TEST(game, initializes_opening_and_first_turn)
{
    // Arrange
    const game game{piece::Black};
    const auto& state = game.state();

    // Assert
    EXPECT_EQ((state.board[BoardCentre, BoardCentre]), space::Black);
    EXPECT_EQ(state.current_player, piece::White);
    EXPECT_EQ(state.captures.pieces_captured_by_black, 0u);
    EXPECT_EQ(state.captures.pieces_captured_by_white, 0u);
    EXPECT_FALSE(state.outcome.has_value());
}

// Invalid moves are rejected.
TEST(game, rejects_invalid_moves)
{
    // Arrange
    game game{piece::Black};

    // Act
    const auto outOfBounds = game.play({GridSize, 0});
    const auto occupied = game.play({BoardCentre, BoardCentre});

    // Assert
    ASSERT_FALSE(outOfBounds.has_value());
    EXPECT_EQ(outOfBounds.error(), move_error::out_of_bounds);
    
    ASSERT_FALSE(occupied.has_value());
    EXPECT_EQ(occupied.error(), move_error::occupied);
}

// Rejected moves must leave the board, captures, turn, and outcome unchanged.
TEST(game, leaves_state_unchanged_after_invalid_move)
{
    // Arrange
    game game{piece::Black};
    const auto& state = game.state();
    
    // Act
    const auto outOfBounds = game.play({GridSize, 0});
    
    // Assert
    EXPECT_EQ(state.current_player, piece::White);
    EXPECT_EQ((state.board[BoardCentre, BoardCentre]), space::Black);
    EXPECT_EQ(state.captures.pieces_captured_by_black, 0u);
    EXPECT_EQ(state.captures.pieces_captured_by_white, 0u);
    EXPECT_FALSE(state.outcome.has_value());
}

// A capture removes both opposing pieces and records one pair for the player.
TEST(game, captures_pair_and_advances_turn)
{
    // Arrange
    game game{make_game_state(piece::White, {
        {{1, 5}, space::Black},
        {{2, 5}, space::Black},
        {{3, 5}, space::White}
    })};
    const auto& state = game.state();

    // Act
    const auto result = game.play({0, 5});

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pairs_captured, 1u);
    EXPECT_FALSE(result->outcome.has_value());

    EXPECT_EQ((state.board[0, 5]), space::White);
    EXPECT_EQ((state.board[1, 5]), space::Empty);
    EXPECT_EQ((state.board[2, 5]), space::Empty);
    EXPECT_EQ((state.board[3, 5]), space::White);
    EXPECT_EQ(state.captures.pieces_captured_by_white, 2u);
    EXPECT_EQ(state.captures.pieces_captured_by_black, 0u);
    EXPECT_EQ(state.current_player, piece::Black);
}

// Captures update the correct total when Black is the player moving.
TEST(game, captures_pair_for_black)
{
    // Arrange
    game game{make_game_state(piece::Black, {
        {{1, 5}, space::White},
        {{2, 5}, space::White},
        {{3, 5}, space::Black}
    })};

    // Act
    const auto result = game.play({0, 5});

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pairs_captured, 1u);
    EXPECT_EQ(game.state().captures.pieces_captured_by_black, 2u);
    EXPECT_EQ(game.state().captures.pieces_captured_by_white, 0u);
    EXPECT_EQ(game.state().current_player, piece::White);
}

// Captures work on all eight rays from the played position.
TEST(game, captures_in_every_direction)
{
    //TODO clean up
    constexpr array<pair<int, int>, 8> directions{{
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {-1, -1}, {1, -1}, {-1, 1}
    }};

    for (const auto [dx, dy] : directions)
    {
        // Arrange
        constexpr coord origin{9, 5};
        const auto along = [=](int distance) {
            return coord{
                static_cast<size_t>(static_cast<int>(origin.x) + distance * dx),
                static_cast<size_t>(static_cast<int>(origin.y) + distance * dy)
            };
        };
        game game{make_game_state(piece::White, {
            {along(1), space::Black},
            {along(2), space::Black},
            {along(3), space::White}
        })};
        const auto& state = game.state();
        
        // Act
        const auto result = game.play(origin);

        // Assert
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->pairs_captured, 1u);

        EXPECT_EQ(state.captures.pieces_captured_by_white, 2u);
        EXPECT_EQ((state.board[origin]), space::White);
        EXPECT_EQ(state.current_player, piece::Black);
    }
}

// A single move may capture a pair on each side of the placed piece.
TEST(game, captures_pairs_on_both_sides_of_a_move)
{
    // Arrange
    game game{make_game_state(piece::White, {
        {{5, 5}, space::White},
        {{6, 5}, space::Black},
        {{7, 5}, space::Black},

        {{9, 5}, space::Black},
        {{10, 5}, space::Black},
        {{11, 5}, space::White}
    })};
    const auto& state = game.state();

    // Act
    const auto result = game.play({8, 5});

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pairs_captured, 2u);
    EXPECT_EQ((state.board[6, 5]), space::Empty);
    EXPECT_EQ((state.board[7, 5]), space::Empty);
    EXPECT_EQ((state.board[9, 5]), space::Empty);
    EXPECT_EQ((state.board[10, 5]), space::Empty);
    EXPECT_EQ(state.captures.pieces_captured_by_white, 4u);
}

// Captures require two opposing pieces and a friendly bracket.
TEST(game, does_not_capture_without_a_bracket)
{
    // Arrange
    game singlePieceGame{make_game_state(piece::White, {
        {{1, 5}, space::Black}
    })};
    game unbracketedPairGame{make_game_state(piece::White, {
        {{1, 5}, space::Black},
        {{2, 5}, space::Black}
    })};

    // Act
    const auto singlePieceResult = singlePieceGame.play({0, 5});
    const auto unbracketedPairResult = unbracketedPairGame.play({0, 5});

    // Assert
    ASSERT_TRUE(singlePieceResult.has_value());
    EXPECT_EQ(singlePieceResult->pairs_captured, 0u);
    EXPECT_EQ((singlePieceGame.state().board[1, 5]), space::Black);
    EXPECT_EQ(singlePieceGame.state().captures.pieces_captured_by_white, 0u);

    ASSERT_TRUE(unbracketedPairResult.has_value());
    EXPECT_EQ(unbracketedPairResult->pairs_captured, 0u);
    EXPECT_EQ((unbracketedPairGame.state().board[1, 5]), space::Black);
    EXPECT_EQ((unbracketedPairGame.state().board[2, 5]), space::Black);
    EXPECT_EQ(unbracketedPairGame.state().captures.pieces_captured_by_white, 0u);

//    game edgeGame{make_game_state(piece::White, {})};
//    const auto edgeMove = edgeGame.play({GridSize - 1, 5});
//
//    ASSERT_TRUE(edgeMove.has_value());
//    EXPECT_EQ(edgeMove->pairs_captured, 0u);
//    EXPECT_EQ(edgeGame.state().captures.pieces_captured_by_white, 0u);
}

// A line win ends the game and prevents subsequent moves.
TEST(game, wins_by_five_in_a_row_and_rejects_later_moves)
{
    // Arrange
    game game{make_game_state(piece::Black, {
        {{7, 9}, space::Black},
        {{8, 9}, space::Black},
        {{9, 9}, space::Black},
        {{10, 9}, space::Black}
    })};
    const auto& state = game.state();

    // Act
    const auto winningMove = game.play({11, 9});
    const auto rejected = game.play({12, 9});

    // TODO chaining expected/optionals support?

    // Assert
    ASSERT_TRUE(winningMove.has_value());
    ASSERT_TRUE(winningMove->outcome.has_value());
    ASSERT_TRUE(winningMove->outcome->winner.has_value());
    EXPECT_EQ(*winningMove->outcome->winner, piece::Black);
    EXPECT_TRUE(has_reason(winningMove->outcome->reasons, win_reason::five_in_a_row));
    EXPECT_FALSE(has_reason(winningMove->outcome->reasons, win_reason::captures));
    EXPECT_EQ(state.current_player, piece::Black);

    ASSERT_FALSE(rejected.has_value());
    EXPECT_EQ(rejected.error(), move_error::game_finished);
    EXPECT_EQ((state.board[12, 9]), space::Empty);
}

// Diagonal lines are checked with the same five-in-a-row rule.
TEST(game, wins_by_diagonal_five_in_a_row)
{
    // Arrange
    game game{make_game_state(piece::White, {
        {{2, 4}, space::White},
        {{3, 5}, space::White},
        {{4, 6}, space::White},
        {{5, 7}, space::White}
    })};

    // Act
    const auto finalMove = game.play({6, 8});

    // Assert
    ASSERT_TRUE(finalMove.has_value());
    ASSERT_TRUE(finalMove->outcome.has_value());
    ASSERT_TRUE(finalMove->outcome->winner.has_value());
    EXPECT_EQ(*finalMove->outcome->winner, piece::White);
    EXPECT_TRUE(has_reason(finalMove->outcome->reasons, win_reason::five_in_a_row));
}

// A line beginning at the edge still counts as five contiguous pieces.
TEST(game, recognizes_five_in_a_row_at_the_board_edge)
{
    // Arrange
    game game{make_game_state(piece::Black, {
        {{0, 0}, space::Black},
        {{1, 0}, space::Black},
        {{2, 0}, space::Black},
        {{3, 0}, space::Black}
    })};

    // Act
    const auto finalMove = game.play({4, 0});

    // Assert
    ASSERT_TRUE(finalMove.has_value());
    ASSERT_TRUE(finalMove->outcome.has_value());
    ASSERT_TRUE(finalMove->outcome->winner.has_value());
    EXPECT_EQ(*finalMove->outcome->winner, piece::Black);
    EXPECT_TRUE(has_reason(finalMove->outcome->reasons, win_reason::five_in_a_row));
}

// Four contiguous pieces are not enough to end the game.
TEST(game, does_not_win_with_only_four_in_a_row)
{
    // Arrange
    game game{make_game_state(piece::White, {
        {{2, 4}, space::White},
        {{3, 5}, space::White},
        {{4, 6}, space::White}
    })};

    // Act
    const auto fourthPiece = game.play({5, 7});

    // Assert
    ASSERT_TRUE(fourthPiece.has_value());
    EXPECT_FALSE(fourthPiece->outcome.has_value());
    EXPECT_FALSE(game.state().outcome.has_value());
    EXPECT_EQ(game.state().current_player, piece::Black);
}

// Reaching ten captured pieces wins even when the move makes no line.
TEST(game, wins_by_capture_threshold_without_five_in_a_row)
{
    // Arrange
    game game{make_game_state(piece::White, {
        {{1, 5}, space::Black},
        {{2, 5}, space::Black},
        {{3, 5}, space::White}
    }, {.pieces_captured_by_white = 8})};

    // Act
    const auto winningMove = game.play({0, 5});

    // Assert
    ASSERT_TRUE(winningMove.has_value());
    ASSERT_TRUE(winningMove->outcome.has_value());
    ASSERT_TRUE(winningMove->outcome->winner.has_value());
    EXPECT_EQ(*winningMove->outcome->winner, piece::White);
    EXPECT_TRUE(has_reason(winningMove->outcome->reasons, win_reason::captures));
    EXPECT_FALSE(has_reason(winningMove->outcome->reasons, win_reason::five_in_a_row));
    EXPECT_EQ(game.state().captures.pieces_captured_by_white, 10u);
    EXPECT_EQ(game.state().current_player, piece::White);
}

// Capturing two pairs may pass the threshold and still ends the game by captures alone.
TEST(game, wins_when_captures_pass_threshold)
{
    // Arrange
    game game{make_game_state(piece::White, {
        {{5, 5}, space::White},
        {{6, 5}, space::Black},
        {{7, 5}, space::Black},

        {{9, 5}, space::Black},
        {{10, 5}, space::Black},
        {{11, 5}, space::White}
    }, {.pieces_captured_by_white = 8})};

    // Act
    const auto winningMove = game.play({8, 5});

    // Assert
    ASSERT_TRUE(winningMove.has_value());
    ASSERT_EQ(winningMove->pairs_captured, 2u);
    ASSERT_TRUE(winningMove->outcome.has_value());
    ASSERT_TRUE(winningMove->outcome->winner.has_value());
    EXPECT_EQ(*winningMove->outcome->winner, piece::White);
    EXPECT_TRUE(has_reason(winningMove->outcome->reasons, win_reason::captures));
    EXPECT_FALSE(has_reason(winningMove->outcome->reasons, win_reason::five_in_a_row));
    EXPECT_EQ(game.state().captures.pieces_captured_by_white, 12u);
    EXPECT_EQ(game.state().current_player, piece::White);
}

// A full board without a winning line is a draw.
TEST(game, ends_in_a_draw_when_the_board_is_full)
{
    //TODO tidy
    
    // Arrange
    auto initialState = make_game_state(piece::Black, {});
    for (size_t y = 0; y < GridSize; ++y)
    {
        for (size_t x = 0; x < GridSize; ++x)
        {
            if (x == 0 && y == 0)
            {
                continue;
            }

            initialState.board[x, y] = (x + 2 * y) % 5 == 0
                ? space::Black
                : space::White;
        }
    }
    game game{initialState};

    // Act
    const auto finalMove = game.play({0, 0});

    // Assert
    ASSERT_TRUE(finalMove.has_value());
    ASSERT_TRUE(finalMove->outcome.has_value());
    EXPECT_FALSE(finalMove->outcome->winner.has_value());
    EXPECT_EQ(finalMove->outcome->reasons, win_reason::none);
    EXPECT_EQ((game.state().board[0, 0]), space::Black);
    EXPECT_EQ(game.state().current_player, piece::Black);
}

// A move that meets both win conditions retains both reasons.
TEST(game, records_simultaneous_capture_and_line_wins)
{
    // Arrange
    game game{make_game_state(piece::White, {
        {{0, 12}, space::White},
        {{0, 13}, space::White},
        {{0, 14}, space::White},
        {{0, 15}, space::White},

        {{1, 16}, space::Black},
        {{2, 16}, space::Black},
        {{3, 16}, space::White}
    }, {.pieces_captured_by_white = 8})};

    // Act
    const auto finalMove = game.play({0, 16});

    // Assert
    ASSERT_TRUE(finalMove.has_value());
    ASSERT_EQ(finalMove->pairs_captured, 1u);
    ASSERT_TRUE(finalMove->outcome.has_value());
    ASSERT_TRUE(finalMove->outcome->winner.has_value());
    EXPECT_EQ(*finalMove->outcome->winner, piece::White);
    EXPECT_TRUE(has_reason(finalMove->outcome->reasons, win_reason::captures));
    EXPECT_TRUE(has_reason(finalMove->outcome->reasons, win_reason::five_in_a_row));
    EXPECT_EQ(game.state().captures.pieces_captured_by_white, 10u);
    EXPECT_EQ(game.state().current_player, piece::White);
}

//TODO experiment with a board initialiser that can be written {{B, W, W, B},{_, _, _, _},...} and be copied directly into the game state.