#include "penteboard.h"

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace cj::pente;
using namespace std;

namespace
{
    space space_from_coords(size_t x, size_t y)
    {
        if (x == y)
        {
            return space::Empty;
        }

        if (x < y)
        {
            return space::Black;
        }

        return space::White;
    }

    void require(bool condition, const string& message)
    {
        if (!condition)
        {
            throw runtime_error{message};
        }
    }

    void test_opposite_and_to_space()
    {
        require(opposite(piece::Black) == piece::White, "opposite(Black) should be White");
        require(opposite(piece::White) == piece::Black, "opposite(White) should be Black");

        require(to_space(piece::Black) == space::Black, "to_space(Black) should be space::Black");
        require(to_space(piece::White) == space::White, "to_space(White) should be space::White");
    }

    void test_is_in_bounds()
    {
        require(is_in_bounds(0, 0), "(0,0) should be in bounds");
        require(is_in_bounds(BoardCentre, BoardCentre), "Board centre should be in bounds");
        require(is_in_bounds(GridSize - 1, GridSize - 1), "Last cell should be in bounds");

        require(!is_in_bounds(-1, 0), "(-1,0) should be out of bounds");
        require(!is_in_bounds(0, -1), "(0,-1) should be out of bounds");
        require(!is_in_bounds(GridSize, 0), "(GridSize,0) should be out of bounds");
        require(!is_in_bounds(0, GridSize), "(0,GridSize) should be out of bounds");
    }

    void test_get_rows_matches_indexing()
    {
        board b{};

        for (size_t y = 0; y < GridSize; y++)
        {
            for (size_t x = 0; x < GridSize; x++)
            {
                b[x, y] = space_from_coords(x, y);
            }
        }

        size_t y = 0;
        for (auto row : get_rows(b))
        {
            require(row.size() == GridSize, "Each row span should have GridSize entries");

            for (size_t x = 0; x < GridSize; x++)
            {
                require(row[x] == space_from_coords(x, y), "get_rows row view should match expected row-major coordinate mapping");
                require(row[x] == b[x, y], "get_rows row view should match board_view logical indexing");
            }

            y++;
        }

        require(y == GridSize, "get_rows should produce GridSize rows");
    }

    void test_apply_captures_horizontal()
    {
        board b{};

        b[2, 5] = space::Black;
        b[3, 5] = space::White;
        b[4, 5] = space::White;
        b[5, 5] = space::Black;

        const auto captured = apply_captures(b, 2, 5);

        require(captured == 2u, "One horizontal capture should remove two pieces");
        require(b[3, 5] == space::Empty, "First captured piece should be removed");
        require(b[4, 5] == space::Empty, "Second captured piece should be removed");
    }

    void test_apply_captures_double_direction()
    {
        board b{};

        b[2, 8] = space::Black;
        b[3, 8] = space::White;
        b[4, 8] = space::White;
        
        // placed piece
        b[5, 8] = space::Black;
        
        b[6, 8] = space::White;
        b[7, 8] = space::White;
        b[8, 8] = space::Black;

        const auto captured = apply_captures(b, 5, 8);

        require(captured == 4u, "Captures on both horizontal directions should remove four pieces");
        require(b[3, 8] == space::Empty, "Left captured piece should be removed");
        require(b[4, 8] == space::Empty, "Left captured piece should be removed");
        require(b[6, 8] == space::Empty, "Right captured piece should be removed");
        require(b[7, 8] == space::Empty, "Right captured piece should be removed");
    }

    void test_apply_captures_diagonal()
    {
        board b{};

        b[4, 4] = space::White;
        b[5, 5] = space::Black;
        b[6, 6] = space::Black;
        b[7, 7] = space::White;

        const auto captured = apply_captures(b, 4, 4);

        require(captured == 2u, "One falling-diagonal capture should remove two pieces");
        require(b[5, 5] == space::Empty, "Diagonal captured piece should be removed");
        require(b[6, 6] == space::Empty, "Diagonal captured piece should be removed");
    }

    void test_apply_captures_no_capture()
    {
        board b{};

        b[0, 0] = space::Black;
        b[1, 0] = space::White;
        b[2, 0] = space::White;

        const auto captured = apply_captures(b, 0, 0);

        require(captured == 0u, "Open pattern at edge should not capture");
        require(b[1, 0] == space::White, "No capture should leave board unchanged");
        require(b[2, 0] == space::White, "No capture should leave board unchanged");
    }

    void test_add_captured_pieces_accumulates_for_black_without_win()
    {
        capture_pots pots{};

        const auto [totalCapturedPieces, hasWonByCaptures] = add_captured_pieces(pots, piece::Black, 2u);

        require(totalCapturedPieces == 2u, "Black captured count should increase by piecesToAdd");
        require(!hasWonByCaptures, "Two captured pieces should not yet win by captures");
        require(pots.pieces_captured_by_black == 2u, "Black pot should be updated");
        require(pots.pieces_captured_by_white == 0u, "White pot should remain unchanged");
    }

    void test_add_captured_pieces_reaches_capture_win_threshold()
    {
        capture_pots pots{};
        pots.pieces_captured_by_white = 8u;

        const auto [totalCapturedPieces, hasWonByCaptures] = add_captured_pieces(pots, piece::White, 2u);

        require(totalCapturedPieces == 10u, "Captured total should include previous pieces and new pieces");
        require(hasWonByCaptures, "Ten captured pieces should win by captures");
        require(pots.pieces_captured_by_black == 0u, "Black pot should remain unchanged");
        require(pots.pieces_captured_by_white == 10u, "White pot should be updated to the winning total");
    }

    void test_add_captured_pieces_passes_capture_win_threshold()
    {
        capture_pots pots{};
        pots.pieces_captured_by_white = 8u;

        const auto [totalCapturedPieces, hasWonByCaptures] = add_captured_pieces(pots, piece::White, 4u);

        require(totalCapturedPieces == 12u, "Captured total should include previous pieces and new pieces");
        require(hasWonByCaptures, "Twelve captured pieces should win by captures");
        require(pots.pieces_captured_by_black == 0u, "Black pot should remain unchanged");
        require(pots.pieces_captured_by_white == 12u, "White pot should be updated to the winning total");
    }

    void test_check_five_in_a_row_horizontal()
    {
        board b{};

        for (size_t x = 3; x <= 7; ++x)
        {
            b[x, 9] = space::Black;
        }

        // placed piece is central in the five-in-a-row
        require(check_five_in_a_row(b, 5, 9), "Five contiguous horizontal pieces should win");
    }

    void test_check_five_in_a_row_diagonal()
    {
        board b{};

        for (size_t i = 0; i < 5; ++i)
        {
            b[2 + i, 4 + i] = space::White;
        }

        // placed piece is central in the five-in-a-row
        require(check_five_in_a_row(b, 4, 6), "Five contiguous diagonal pieces should win");
    }

    void test_check_five_in_a_row_edge_clamped()
    {
        board b{};

        for (size_t x = 0; x < 5; ++x)
        {
            b[x, 0] = space::Black;
        }

        // placed piece is at the edge of the five-in-a-row
        require(check_five_in_a_row(b, 0, 0), "Edge-aligned five should be detected without out-of-bounds access");
    }

    void test_check_five_in_a_row_not_enough()
    {
        board b{};

        for (size_t y = 7; y < 11; ++y)
        {
            b[12, y] = space::White;
        }

        // placed piece is central in the four-in-a-row
        require(!check_five_in_a_row(b, 12, 9), "Four contiguous pieces should not win");
    }
}

int main()
{
    try
    {
        test_opposite_and_to_space();
        test_is_in_bounds();
        test_get_rows_matches_indexing();
        test_apply_captures_horizontal();
        test_apply_captures_double_direction();
        test_apply_captures_diagonal();
        test_apply_captures_no_capture();
        test_add_captured_pieces_accumulates_for_black_without_win();
        test_add_captured_pieces_reaches_capture_win_threshold();
        test_add_captured_pieces_passes_capture_win_threshold();
        test_check_five_in_a_row_horizontal();
        test_check_five_in_a_row_diagonal();
        test_check_five_in_a_row_edge_clamped();
        test_check_five_in_a_row_not_enough();
    }
    catch (const exception& ex)
    {
        cerr << "Test failure: " << ex.what() << endl;
        return 1;
    }

    cout << "All penteboard tests passed\n";
    return 0;
}
