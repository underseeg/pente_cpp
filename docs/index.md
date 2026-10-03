# Pente {#mainpage}

## Introduction

This is a Modern C++ implementation of the game Pente. It supports two players via a command line interface. This minimal documentation is more an example of good practice than for any practical purpose.

## Code reference

See the [pente namespace](@ref cj::pente)

## Usage Overview

`game` owns and updates a `game_state`. Construction places the chosen starting colour at the centre and makes the opposite colour the first to play.

```cpp
cj::pente::game game{cj::pente::piece::Black};
```

Apply a move through the game. Illegal moves return a `move_error` and leave the state unchanged. A successful move reports captured pairs and any resulting outcome.

```cpp
auto move = game.play(position);
if (!move) {
    // Handle move.error(): out_of_bounds, occupied, or game_finished.
}
else if (move->pairs_captured > 0) {
    // Report captures for this move.
}

// If appropriate, handle the final outcome of the game.
if (move->outcome) {
    // A draw is represented by winner == std::nullopt.
    // The win_reason may contain both captures and five_in_a_row.
}
```

Read the current state by const reference to display the board, capture totals, current player, and final outcome. As it's a reference it will reflect subsequent moves; copy-by-value the `game_state` if needed.

```cpp
const auto& state = game.get_current_state();
for (const auto row : state.board_state.rows()) {
    // Display each row.
}
```

The game ends when a player captures five pairs, makes five or more contiguous pieces, or fills the board without a winner. See `run_game()` in `src/main.cpp` for the command-line flow.
