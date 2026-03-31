#include <chrono>
#include "sudoku.h"

int main()
{
    auto start = std::chrono::steady_clock::now();

    Sudoku::Game sudoku{};
    // load the board from the given puzzle.txt file
    bool flag = sudoku.load_file_to_board("puzzle.txt");

    if (flag)
    {
        std::cout << "Board has been loaded\n";
        Sudoku::Solver solver;
        solver.solve(sudoku);
        sudoku.print_board();
    }
    else
    {
        sudoku.print_board();
        std::cout << "Invalid board. Please check!\n";
    }

    auto end = std::chrono::steady_clock::now();
    // Store the time difference between start and end
    auto diff = end - start;
    std::cout << "Time taken to execute: " << std::chrono::duration<double, std::milli>(diff).count() << " ms" << '\n';
}
