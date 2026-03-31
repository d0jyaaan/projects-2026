#ifndef SUDOKU_H
#define SUDOKU_H

#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <utility>
#include <algorithm>

namespace Sudoku
{
    class Game
    {
    private:
        bool check_board()
        {
            // check validity of loaded board
            for (int row = 0; row < 9; row++)
            {
                for (int col = 0; col < 9; col++)
                {
                    if (board[row][col] != '.')
                    {
                        if (!check_square(row, col))
                            return false;
                    }
                }
            }
            return true;
        }

        bool check_square(int row, int col)
        {
            // make copy of board and cell table
            auto boardCopy = board;
            auto cellCopy = cell;

            // remove the number on square to be checked
            boardCopy[row][col] = '.';
            cellCopy[int(row / 3) * 3 + col / 3][(row % 3) * 3 + (col % 3)] = '.';

            // check cell and row for duplicate
            if (cellCopy[int(row / 3) * 3 + col / 3].find(board[row][col]) != std::string::npos ||
                boardCopy[row].find(board[row][col]) != std::string::npos)
                return false;

            // check column for duplicates
            for (int i = 0; i < 8; i++)
            {
                if (boardCopy[i][col] == board[row][col])
                    return false;
            }
            return true;
        }

    public:
        // 9x9 board with a total of 81 squares
        std::vector<std::string> board = std::vector<std::string>(9);
        // 9 cells with 9 squares each
        std::vector<std::string> cell = std::vector<std::string>(9);

        std::string board_to_string()
        {
            // convert 9x9 board into 81 length string
            std::string s;
            for (int row = 0; row < 9; row++)
            {
                for (int col = 0; col < 9; col++)
                {
                    s += board[row][col];
                }
            }
            return s;
        }

        std::vector<std::string> string_to_board(std::string &stringCopy)
        {
            // convert 81 length string to 9x9 board
            std::vector<std::string> v = std::vector<std::string>(9);
            for (int i = 0; i < 9; i++)
            {
                for (int j = 0; j < 9; j++)
                {
                    v[i] += stringCopy[i * 9 + j];
                }
            }
            return v;
        }

        std::vector<std::string> board_to_cell()
        {
            // convert 9x9 board to cell
            std::vector<std::string> cellCopy = std::vector<std::string>(9);
            for (int i = 0; i < 9; i++)
            {
                for (int j = 0; j < 9; j++)
                {
                    cellCopy[int(i / 3) * 3 + j / 3] += board[i][j];
                }
            }
            return cellCopy;
        }

        bool load_file_to_board(std::string filepath)
        {
            // read file and parse it to form a 9 x 9 board with 3 x 3 cells
            std::ifstream file(filepath);
            std::string line;

            int index{0};
            while (std::getline(file, line))
            {
                for (int i = 0; i < line.size(); i++)
                {
                    // add number / blank to row
                    board[index] += line[i];
                    // add to cell
                    // for every 3 rows, go to the next row of cells
                    cell[int(index / 3) * 3 + i / 3] += line[i];
                }
                index++;
            }
            // check whether the loaded board is valid
            return check_board();
        }

        void print_board()
        {
            for (int row = 0; row < 9; row++)
            {
                for (int col = 0; col < 9; col++)
                {
                    std::cout << board[row][col] << ' ';
                    if ((col % 3 == 2) && (col != 8))
                    {
                        std::cout << "| ";
                    }
                }
                if ((row % 3 == 2) && (row != 8))
                {
                    std::cout << '\n'
                              << "======+=======+======";
                }
                std::cout << '\n';
            }
        }
    };

    class Solver
    {
        // given a state of the board, evaluate / solve it

    public:
        bool check_solved(Game &gameState)
        {
            // check all 81 squares whether the numbers assigned is valid
            // it is assumed that all cells are already filled with a number
            for (int row = 0; row < 9; row++)
            {
                for (int col = 0; col < 9; col++)
                {
                    if (!check_valid(gameState, row, col))
                        return false;
                }
            }
            return true;
        }

        bool check_valid(Game &gameState, int row, int col)
        {
            // given a row, column and board state, determine whether the number assigned to this square is valid
            char number{static_cast<char>(gameState.board[row][col])};

            int cellIndex{int(row / 3) * 3 + col / 3};
            int indexInCell{(row % 3) * 3 + (col % 3)};
            for (int i = 0; i < 9; i++)
            {
                // check row validity
                if (gameState.board[row][i] == number && i != col)
                    return false;

                // check column validity
                if (gameState.board[i][col] == number && i != row)
                    return false;

                // check cell validity
                if (gameState.cell[cellIndex][i] == number && i != indexInCell)
                    return false;
            }
            return true;
        }

        bool check_filled(Game &gameState)
        {
            // check whether all 81 squares is filled with a number
            for (auto row : gameState.board)
            {
                if (row.find('.') != std::string::npos)
                    return false;
            }
            return true;
        }

        bool check_tally(Game &gameState, std::map<std::pair<int, int>, std::set<int>> &preemptive)
        {
            // check whether the number of empty squares and size of preemptive matches, if not, return false
            int count{0};
            for (auto row : gameState.board)
            {
                for (auto character : row)
                {
                    if (character == '.')
                        count++;
                }
            }
            if (count == preemptive.size())
                return true;

            return false;
        }

        bool check_contradiction(Game &gameState, std::map<std::pair<int, int>, std::set<int>> &preemptive)
        {
            // given a gamestate check whether are there any contradictions within the puzzle
            // Note: Check everystep for contradiction. Stop when start bruteforcing
            // Given that a contradicting square will not occur since 'fill' function doesnt allow this

            // Contradiction : In row / column / cell, missing this number, but preemptive dont have
            // If rowRemain, colRemain and cellRemain are empty, means gamestate is valid
            std::vector<std::set<int>> rowRemain{9, std::set<int>{1, 2, 3, 4, 5, 6, 7, 8, 9}};
            std::vector<std::set<int>> colRemain{9, std::set<int>{1, 2, 3, 4, 5, 6, 7, 8, 9}};
            std::vector<std::set<int>> cellRemain{9, std::set<int>{1, 2, 3, 4, 5, 6, 7, 8, 9}};

            for (int i = 0; i < 9; i++)
            {
                // check row for contradictions
                for (auto x : gameState.board[i])
                {
                    if (x != '.')
                        rowRemain[i].erase(x - '0');
                }

                // check cells for contradiction
                for (auto x : gameState.cell[i])
                {
                    if (x != '.')
                        cellRemain[i].erase(x - '0');
                }

                // check column for contradiction
                for (int j = 0; j < 9; j++)
                {
                    if (gameState.board[j][i] != '.')
                    {
                        colRemain[i].erase(gameState.board[j][i] - '0');
                    }
                }
            }

            // check preemptives and remove the appearing numbers from remain tables
            for (auto x : preemptive)
            {
                int row{x.first.first};
                int col{x.first.second};

                for (int number : x.second)
                {
                    rowRemain[row].erase(number);
                    colRemain[col].erase(number);
                    cellRemain[int(row / 3) * 3 + col / 3].erase(number);
                }
            }

            // check whether all rowRemain, colRemain, cellRemain are empty. if still got number. return false at first instance
            for (int i = 0; i < 9; i++)
            {
                if (rowRemain[i].size() != 0 || colRemain[i].size() != 0 || cellRemain[i].size() != 0)
                    return false;
            }
            return true;
        }

        std::map<std::pair<int, int>, std::set<int>> generate_preemp(Game &gameState)
        {
            // given a game state, generate the preemptive table
            std::map<std::pair<int, int>, std::set<int>> preemptive;
            for (int row = 0; row < 9; row++)
            {
                for (int col = 0; col < 9; col++)
                {
                    if (gameState.board[row][col] == '.')
                        get_preemp(gameState, preemptive, row, col);
                }
            }
            return preemptive;
        }

        void get_preemp(Game &gameState, std::map<std::pair<int, int>, std::set<int>> &preemptive, int row, int col)
        {
            int cellIndex{int(row / 3) * 3 + col / 3};
            std::pair<int, int> address{std::make_pair(row, col)};
            if (preemptive.count(address) == 0)
                preemptive[address] = std::set<int>{1, 2, 3, 4, 5, 6, 7, 8, 9};

            // std::set<int> preempSet{(preemptive.count(address) == 0) ? std::set<int>{1, 2, 3, 4, 5, 6, 7, 8, 9} : preemptive[address]};
            // go through the preemptive, if invalid then remove
            for (int i = 1; i <= 9; i++)
            {
                // if is not in the preemptive list, then dont need to check this integer
                if (preemptive[address].count(i) == 0)
                    continue;

                std::string number{std::to_string(i)};
                // check cell and row to see if number is valid
                if (gameState.cell[cellIndex].find(number) != std::string::npos ||
                    gameState.board[row].find(number) != std::string::npos)
                {
                    preemptive[address].erase(i);
                    continue;
                }

                // check column to see if number is valid
                bool flagInvalid{false};
                for (int j = 0; j < 9; j++)
                {
                    if (gameState.board[j][col] == number[0])
                    {
                        flagInvalid = true;
                        break;
                    }
                }
                if (flagInvalid)
                    preemptive[address].erase(i);
            }
        }

        void update_preemp(Game &gameState, std::map<std::pair<int, int>, std::set<int>> &preemptive)
        {
            // update the preemptive list for all unfilled squares
            // (unfilled squares preemptive will be in preemptive list already)
            for (auto it = preemptive.begin(); it != preemptive.end();)
            {
                if ((*it).second.size() == 0)
                {
                    it = preemptive.erase(it);
                }
                else
                {
                    get_preemp(gameState, preemptive, (*it).first.first, (*it).first.second);
                    ++it;
                }
            }
        }

        void print_preemp(std::map<std::pair<int, int>, std::set<int>> &preemptive)
        {
            // print the preemptive list
            for (auto x : preemptive)
            {
                std::cout << "(" << x.first.first << "," << x.first.second << "): ";

                for (auto y : x.second)
                {
                    std::cout << y << ' ';
                }
                std::cout << '\n';
            }
        }

        void fill(Game &gameState, int &row, int &col, std::string &toBeAssigned)
        {
            // assign the board coordinate as the only element in set
            gameState.board[row][col] = toBeAssigned[0];
            // assign to cell coordinate as well
            gameState.cell[int(row / 3) * 3 + col / 3][(row % 3) * 3 + (col % 3)] = toBeAssigned[0];
        }

        bool fill_singles(Game &gameState, std::map<std::pair<int, int>, std::set<int>> &preemptive)
        {
            bool isFilled{false};
            for (auto it = preemptive.rbegin(); it != preemptive.rend(); it++)
            {
                int row{(*it).first.first};
                int col{(*it).first.second};

                // fill the empty square if preemptive size is 1
                if ((*it).second.size() == 1)
                {
                    std::string toBeAssigned{std::to_string(*(*it).second.begin())};
                    // fill the square
                    fill(gameState, row, col, toBeAssigned);
                    // erase coordinate from preemptive list
                    preemptive.erase((*it).first);
                    // update flag when fill action occured
                    isFilled = true;
                }
            }
            return isFilled;
        }

        bool solve(Game &gameState)
        {
            // get the preemp for this game state
            std::map<std::pair<int, int>, std::set<int>> preemptive{generate_preemp(gameState)};

            // while the preemptive list is not empty, means the game is not yet completed
            while (preemptive.size() != 0)
            {
                // if any square has been filled, means game can still continue
                // after filling all squares that have 1 available option, update all remaining preemptives
                if (!check_contradiction(gameState, preemptive))
                {
                    // std::cout << "Contradiction\n";
                    return false;
                }

                if (fill_singles(gameState, preemptive))
                    update_preemp(gameState, preemptive);

                // if no cells have been filled on previous move, need to use trial and error to solve
                else
                {
                    std::string answer;
                    // solved
                    if (search_path(gameState, preemptive, answer))
                    {
                        // std::cout << "Solved\n";
                        gameState.board = gameState.string_to_board(answer);
                        // gameState.print_board();
                        return true;
                    }
                    // puzzle cant be solved
                    else
                    {
                        std::cout << "This sudoku puzzle can't be solved\n";
                        // gameState.print_board();
                        return false;
                    }

                    // the only 2 results are either solved / failed, thus break from loop
                    break;
                }
            }
            return false;
        }

        std::vector<std::pair<int, int>> get_queue(std::map<std::pair<int, int>, std::set<int>> &preemptive)
        {
            // given a preemptive list, generate a queue of square coordinates sorted by their size of preemptive numbers list
            std::vector<std::pair<std::pair<int, int>, int>> weightage;
            std::vector<std::pair<int, int>> queue;

            // get the weightage of each square coordinate
            for (auto it = std::next(preemptive.begin()); it != preemptive.end(); ++it)
            {
                weightage.push_back(std::make_pair(it->first, (it->second).size()));
            }
            // sort the square coordinates by weightage
            std::sort(weightage.begin(), weightage.end(), [](const auto &a, const auto &b)
                      { return a.second > b.second; });

            for (auto x : weightage)
            {
                queue.push_back(x.first);
            }
            return queue;
        }

        bool search_path(Game gameState, std::map<std::pair<int, int>, std::set<int>> preemptive, std::string &answer)
        {
            // given an unsolved board, go through all feasible preemps
            // if all preemps have been explored, means theres no solution. return false
            std::vector<std::pair<int, int>> queue = get_queue(preemptive);

            while (queue.size() != 0)
            {
                // get the coord with the smallest size of preemptive
                std::pair<int, int> least{queue.back()};
                queue.pop_back();

                for (auto x : preemptive[least])
                {
                    // try filling cell with a value and determine whether game can be solved from this state
                    std::string xString{std::to_string(x)};
                    fill(gameState, least.first, least.second, xString);

                    // make a copy of the given preemptive
                    std::map<std::pair<int, int>, std::set<int>> preemptiveCopy{preemptive};

                    // remove coord with smallest size of preemptive from preemptive copy
                    preemptiveCopy.erase(least);

                    // update preemptive copy
                    update_preemp(gameState, preemptiveCopy);

                    // update all the cells that only have a single preemp
                    while (fill_singles(gameState, preemptiveCopy))
                        update_preemp(gameState, preemptiveCopy);

                    // clean up preemptive copy by deleting keys with empty preemp sets
                    update_preemp(gameState, preemptiveCopy);

                    // check whether size of preemptive is same as number of empty squares on board
                    if (!check_tally(gameState, preemptiveCopy))
                        continue;

                    // recursive algorithm: brute force until solution is found
                    if (search_path(gameState, preemptiveCopy, answer))
                        return true;
                }

                if (queue.size() == 0)
                    gameState.board[least.first][least.second] = '.';
            }

            if (check_solved(gameState) && check_filled(gameState))
            {
                answer = gameState.board_to_string();
                return true;
            }
            return false;
        }
    };
}

#endif
