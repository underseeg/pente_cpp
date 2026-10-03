#include "pentegame.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <utility>

using namespace cj::pente;
using namespace std;

namespace
{
    constexpr auto B = space::Black;
    constexpr auto W = space::White;
    constexpr auto _ = space::Empty;

    template <std::size_t Height, std::size_t Width>
    game_state make_game_state(
        piece currentPlayer,
        const space (&layout)[Height][Width],
        capture_pots captures = {})
    {
        game_state initialState{};
        initialState.current_player = currentPlayer;
        initialState.captures = captures;
        for (auto [destinationRow, sourceRow] : views::zip(initialState.board.rows(), layout))
        {
            ranges::copy(sourceRow, destinationRow.begin());
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
    space layout[1][4] = {{_, B, B, W}};
    game game{make_game_state(piece::White, layout)};
    const auto& state = game.state();

    // Act
    const auto result = game.play({0, 0});

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pairs_captured, 1u);
    EXPECT_FALSE(result->outcome.has_value());

    EXPECT_EQ((state.board[0, 0]), space::White);
    EXPECT_EQ((state.board[1, 0]), space::Empty);
    EXPECT_EQ((state.board[2, 0]), space::Empty);
    EXPECT_EQ((state.board[3, 0]), space::White);
    EXPECT_EQ(state.captures.pieces_captured_by_white, 2u);
    EXPECT_EQ(state.captures.pieces_captured_by_black, 0u);
    EXPECT_EQ(state.current_player, piece::Black);
}

// Captures update the correct total when Black is the player moving.
TEST(game, captures_pair_for_black)
{
    // Arrange
    space layout[1][4] = {{_, W, W, B}};
    game game{make_game_state(piece::Black, layout)};

    // Act
    const auto result = game.play({0, 0});

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
    // Arrange
    space layout[7][7]{
        {W, _, _, W, _, _, W},
        {_, B, _, B, _, B, _},
        {_, _, B, B, B, _, _},
        {W, B, B, _, B, B, W},
        {_, _, B, B, B, _, _},
        {_, B, _, B, _, B, _},
        {W, _, _, W, _, _, W}
    };
    game game{make_game_state(piece::White, layout)};
    const auto& state = game.state();

    // Act
    const auto result = game.play({3, 3});

    // Assert
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->outcome.has_value());
    EXPECT_EQ(result->outcome->winner, piece::White);
    EXPECT_EQ(result->outcome->reasons, win_reason::captures);
    EXPECT_EQ(result->pairs_captured, 8u);
    EXPECT_EQ(state.captures.pieces_captured_by_white, 16u);
    EXPECT_EQ(state.captures.pieces_captured_by_black, 0u);
}

// A single move may capture a pair on each side of the placed piece.
TEST(game, captures_pairs_on_both_sides_of_a_move)
{
    // Arrange
    space layout[1][7] = {{W, B, B, _, B, B, W}};
    game game{make_game_state(piece::White, layout)};
    const auto& state = game.state();

    // Act
    const auto result = game.play({3, 0});

    // Assert
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->pairs_captured, 2u);
    EXPECT_EQ((state.board[1, 0]), space::Empty);
    EXPECT_EQ((state.board[2, 0]), space::Empty);
    EXPECT_EQ((state.board[4, 0]), space::Empty);
    EXPECT_EQ((state.board[5, 0]), space::Empty);
    EXPECT_EQ(state.captures.pieces_captured_by_white, 4u);
}

// Captures require two opposing pieces and a friendly bracket.
TEST(game, does_not_capture_without_a_bracket)
{
    // Arrange
    space singlePieceLayout[1][2] = {{_, B}};
    game singlePieceGame{make_game_state(piece::White, singlePieceLayout)};

    space unbracketedPairLayout[1][3] = {{_, B, B}};
    game unbracketedPairGame{make_game_state(piece::White, unbracketedPairLayout)};

    // Act
    const auto singlePieceResult = singlePieceGame.play({0, 0});
    const auto unbracketedPairResult = unbracketedPairGame.play({0, 0});

    // Assert
    ASSERT_TRUE(singlePieceResult.has_value());
    EXPECT_EQ(singlePieceResult->pairs_captured, 0u);
    EXPECT_EQ((singlePieceGame.state().board[1, 0]), space::Black);
    EXPECT_EQ(singlePieceGame.state().captures.pieces_captured_by_white, 0u);

    ASSERT_TRUE(unbracketedPairResult.has_value());
    EXPECT_EQ(unbracketedPairResult->pairs_captured, 0u);
    EXPECT_EQ((unbracketedPairGame.state().board[1, 0]), space::Black);
    EXPECT_EQ((unbracketedPairGame.state().board[2, 0]), space::Black);
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
    space layout[1][6] = {{B, B, B, B, _, _}};
    game game{make_game_state(piece::Black, layout)};
    const auto& state = game.state();

    // Act
    const auto winningMove = game.play({4, 0});
    const auto rejected = game.play({5, 0});

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
    EXPECT_EQ((state.board[5, 0]), space::Empty);
}

// Diagonal lines are checked with the same five-in-a-row rule.
TEST(game, wins_by_diagonal_five_in_a_row)
{
    // Arrange
    space layout[5][5] = {
        {W, _, _, _, _},
        {_, W, _, _, _},
        {_, _, W, _, _},
        {_, _, _, W, _},
        {_, _, _, _, _}
    };
    game game{make_game_state(piece::White, layout)};

    // Act
    const auto finalMove = game.play({4, 4});

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
    space layout[1][5] = {{B, B, B, B, _}};
    game game{make_game_state(piece::Black, layout)};

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
    space layout[4][4] = {
        {W, _, _, _},
        {_, W, _, _},
        {_, _, W, _},
        {_, _, _, _}
    };
    game game{make_game_state(piece::White, layout)};

    // Act
    const auto fourthPiece = game.play({3, 3});

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
    space layout[1][4] = {{_, B, B, W}};
    game game{make_game_state(
        piece::White, layout, {.pieces_captured_by_white = 8})};

    // Act
    const auto winningMove = game.play({0, 0});

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
    space layout[1][7] = {{W, B, B, _, B, B, W}};
    game game{make_game_state(
        piece::White, layout, {.pieces_captured_by_white = 8})};

    // Act
    const auto winningMove = game.play({3, 0});

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
    // Arrange
    space layout[GridSize][GridSize]{
        {_, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W},
        {W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B},
        {W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W},
        {W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W},
        {W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W},
        {B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W},
        {W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B},
        {W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W},
        {W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W},
        {W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W},
        {B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W},
        {W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B},
        {W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W},
        {W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W},
        {W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W},
        {B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W},
        {W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B},
        {W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W},
        {W, W, W, W, B, W, W, W, W, B, W, W, W, W, B, W, W, W, W}
    };
    game game{make_game_state(piece::Black, layout)};
    const game_state& state = game.state();

    // Act
    const auto finalMove = game.play({0, 0});

    // Assert
    ASSERT_TRUE(finalMove.has_value());
    ASSERT_TRUE(finalMove->outcome.has_value());
    EXPECT_FALSE(finalMove->outcome->winner.has_value());
    EXPECT_EQ(finalMove->outcome->reasons, win_reason::none);
    EXPECT_EQ((state.board[0, 0]), space::Black);
}

// A move that meets both win conditions retains both reasons.
TEST(game, records_simultaneous_capture_and_line_wins)
{
    // Arrange
    space layout[5][4] = {
        {W, _, _, _},
        {W, _, _, _},
        {W, _, _, _},
        {W, _, _, _},
        {_, B, B, W}
    };
    game game{make_game_state(
        piece::White, layout, {.pieces_captured_by_white = 8})};

    // Act
    const auto finalMove = game.play({0, 4});

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
