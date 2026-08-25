# Pente {#mainpage}

## Introduction

This is a Modern C++ implementation of the game Pente. It supports two players via a command line interface. This minimal documentation is more an example of good practice than for any practical purpose.

## Code reference

See the [pente namespace](@ref cj::pente)

## Usage Overview

This section shows the typical flow when driving gameplay from a caller perspective. For a runnable example, see `run_game()` in `src/main.cpp`.

### Flow summary

```text
Validate move -> Place piece -> Apply captures -> Record captures -> Check five-in-a-row -> Switch turn
```

### Step-by-step

Below are the typical steps a caller performs for each move. To run a game, the following state is needed:

```cpp
cj::pente::board board{};
cj::pente::capture_pots pots{};
cj::pente::piece currentPlayer{piece::Black};
```

#### 1) Validate the move

Having parsed a player's chosen location, confirm the coordinates are on the board and the target space is empty.

```cpp
if (!cj::pente::is_in_bounds(position)) {
    // reject: out of bounds
}
if (board[position] != cj::pente::space::Empty) {
    // reject: space already occupied
}
```

#### 2) Place the piece

Update the board with the player's piece.

```cpp
board[position] = cj::pente::to_space(current);
```

#### 3) Detect & apply pair captures

Call `apply_pair_captures()` to remove any captured opponent pieces. This mutates the board and returns the number of pieces removed by placing a piece at this `position`. Dividing `capturedPieces` by 2 gives the number of pairs captured.

```cpp
unsigned int capturedPieces = cj::pente::apply_pair_captures(board, position);
```

#### 4) Record captures and test capture-win

Update the current player's total in `capture_pots` and test whether they have won by capturing enough pairs.

Both the win condition and the updated total captured pieces are returned in a single call to `add_captured_pieces()`.

```cpp
auto [totalCaptured, hasWonByCaptures] = cj::pente::add_captured_pieces(pots, currentPlayer, capturedPieces);
if (hasWonByCaptures) {
    // current player wins by captures
}
```

#### 5) Check five-in-a-row

Run the five-in-a-row check on the current board, based on the piece played at `position`. If true, the current player has won by five-in-a-row.

```cpp
if (cj::pente::check_five_in_a_row(board, position)) {
    // current player wins by five-in-a-row
}
```

#### 6) Continue the loop

If no win has occurred, switch `currentPlayer` to the opposite player and continue accepting moves.

```cpp
currentPlayer = cj::pente::opposite(currentPlayer);
```

These small snippets map directly to the full flow in `src/main.cpp`.
