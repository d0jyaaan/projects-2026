# Sudoku Solver (2026)

A C++ Sudoku solver built around constraint tracking and backtracking.

The project focuses on representing the board cleanly and solving it step-by-step by narrowing down possible values before falling back to search.

---

## Overview

The solver works in two stages:

1. **Candidate generation (preemptive sets)**  
   For every empty cell, a set of possible values (1–9) is maintained based on:
   - row constraints  
   - column constraints  
   - 3×3 cell constraints  

2. **Solving**
   - Fill cells that have only one possible value  
   - Update remaining candidates  
   - If no direct moves are possible, try values recursively (backtracking)  

---

## Structure

### `Game`

Handles board representation and basic operations.

- `board`  
  9×9 grid stored as `std::vector<std::string>`

- `cell`  
  3×3 subgrids stored as 9 strings for faster lookup

#### Key functions

- `load_file_to_board()`  
  Reads input from file and builds board + cell representation  

- `check_board()`  
  Validates initial board  

- `print_board()`  
  Prints formatted Sudoku grid  

- `board_to_string()` / `string_to_board()`  
  Converts between 2D board and flat string representation  

---

### `Solver`

Handles solving logic.

#### Core concepts

- **Preemptive table**  
Maps each empty cell to its possible values  

- **Constraint updates**  
Candidate sets are continuously refined as values are filled  

---

## Solving Flow

1. Generate preemptive table  
2. Repeat until solved:
 - Fill cells with only one candidate  
 - Update all remaining candidates  
3. If stuck:
 - Pick a cell with the fewest candidates  
 - Try each possibility recursively  
 - Backtrack if contradiction occurs  

---

## Key Functions

- `generate_preemp()`  
Builds candidate sets for all empty cells  

- `fill_singles()`  
Fills cells that only have one valid option  

- `update_preemp()`  
Recomputes candidate sets after changes  

- `check_contradiction()`  
Detects impossible states early  

- `search_path()`  
Recursive backtracking search  

---

## Input / Output

### Input

A text file containing a 9×9 grid:

- Digits `1–9` represent filled cells  
- `.` represents empty cells  

Example:

~~~
5 3 . . 7 . . . .
6 . . 1 9 5 . . .
. 9 8 . . . . 6 .
8 . . . 6 . . . 3
4 . . 8 . 3 . . 1
7 . . . 2 . . . 6
. 6 . . . . 2 8 .
. . . 4 1 9 . . 5
. . . . 8 . . 7 9
~~~

### Output

~~~
5 3 4 | 6 7 8 | 9 1 2
6 7 2 | 1 9 5 | 3 4 8
1 9 8 | 3 4 2 | 5 6 7
======+=======+======
8 5 9 | 7 6 1 | 4 2 3
4 2 6 | 8 5 3 | 7 9 1
7 1 3 | 9 2 4 | 8 5 6
======+=======+======
9 6 1 | 5 3 7 | 2 8 4
2 8 7 | 4 1 9 | 6 3 5
3 4 5 | 2 8 6 | 1 7 9
~~~
## Running

Compile:
~~~
g++ main.cpp -o sudoku
~~~

Run:
~~~
./sudoku
~~~

---

## Notes

- Uses `std::map` and `std::set` for clarity  
- Keeps board and 3×3 cell representations in sync  
- Designed to make intermediate states easy to inspect  

---

## Possible Improvements

- Replace sets with bitmasks for faster lookup  
- Improve heuristic for choosing next cell  
- Add puzzle generator  
- Add difficulty rating  
