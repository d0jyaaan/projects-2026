#include <iostream>
#include <vector>
#include <string>
#include <expected>
#include <cctype>
#include <iomanip>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <map>
#include <cmath>
#include <bitset>
#include <optional>
#include <array>

#ifdef _WIN32
#include <windows.h>
#endif

// |----------Helper datatypes----------|
enum class Encoding : uint8_t
{
    Error = 0,
    Numeric = 1,
    Alphanumeric = 2,
    Byte = 3,
    Kanji = 4,
};

enum class ECLevel : uint8_t
{
    L = 0,
    M = 1,
    Q = 2,
    H = 3
};

// error codes for when generating qr code
enum class ErrorType
{
    IncorrectEncodingModeGiven,
    CharacterLimitExceeded,
    InvalidVersionGiven
};

struct BlockGroup
{
    uint8_t blocks;
    uint16_t codewords;
};

struct QRBlockInfo
{
    uint16_t totalDataCodewords;
    uint8_t ecCodewordsPerBlock;
    BlockGroup g1;
    BlockGroup g2;
};

struct QRinfo
{
    int version;
    Encoding encoding;
    ECLevel eclevel;
};
// |----------Helper datatypes----------|

// |----------Constants----------|
// file path containing the unicode -> shiftJIS mapping
const std::string SHIFT_JIS_FILE_PATH{"ShiftJIS.txt"};

// forward declaration for load_shiftJIS_table() function
std::unordered_map<uint32_t, uint16_t> load_shiftJIS_table();

void print_qr(std::vector<std::vector<int8_t>> &qrcode, bool padding = true);
// Load the SHIFT_JIS_TABLE
const std::unordered_map<uint32_t, uint16_t> SHIFT_JIS_TABLE = load_shiftJIS_table();

// Initialise the functions to build log and antilog table
constexpr std::array<uint8_t, 256> build_EXP();
constexpr std::array<uint8_t, 256> build_LOG();

// Build the log and antilog tables
const std::array<uint8_t, 256> EXP{build_EXP()};
const std::array<uint8_t, 256> LOG{build_LOG()};

const uint8_t finder[7][7] = {
    {1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 1},
    {1, 0, 1, 1, 1, 0, 1},
    {1, 0, 1, 1, 1, 0, 1},
    {1, 0, 1, 1, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1}};

const uint8_t alignment[5][5] = {
    {1, 1, 1, 1, 1},
    {1, 0, 0, 0, 1},
    {1, 0, 1, 0, 1},
    {1, 0, 0, 0, 1},
    {1, 1, 1, 1, 1}};

// Array that specifies how much padding is need for structured message for each QR code version
constexpr std::array<uint8_t, 40> VERSIONPADDING = {
    0, 7, 7, 7, 7, 7, 0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 0};

constexpr std::array<std::array<QRBlockInfo, 4>, 41> EC_TABLE = {
    {{},

     // 1
     {{{19, 7, {1, 19}, {0, 0}}, {16, 10, {1, 16}, {0, 0}}, {13, 13, {1, 13}, {0, 0}}, {9, 17, {1, 9}, {0, 0}}}},
     // 2
     {{{34, 10, {1, 34}, {0, 0}}, {28, 16, {1, 28}, {0, 0}}, {22, 22, {1, 22}, {0, 0}}, {16, 28, {1, 16}, {0, 0}}}},
     // 3
     {{{55, 15, {1, 55}, {0, 0}}, {44, 26, {1, 44}, {0, 0}}, {34, 18, {2, 17}, {0, 0}}, {26, 22, {2, 13}, {0, 0}}}},
     // 4
     {{{80, 20, {1, 80}, {0, 0}}, {64, 18, {2, 32}, {0, 0}}, {48, 26, {2, 24}, {0, 0}}, {36, 16, {4, 9}, {0, 0}}}},
     // 5
     {{{108, 26, {1, 108}, {0, 0}}, {86, 24, {2, 43}, {0, 0}}, {62, 18, {2, 15}, {2, 16}}, {46, 22, {2, 11}, {2, 12}}}},
     // 6
     {{{136, 18, {2, 68}, {0, 0}}, {108, 16, {4, 27}, {0, 0}}, {76, 24, {4, 19}, {0, 0}}, {60, 28, {4, 15}, {0, 0}}}},
     // 7
     {{{156, 20, {2, 78}, {0, 0}}, {124, 18, {4, 31}, {0, 0}}, {88, 18, {2, 14}, {4, 15}}, {66, 26, {4, 13}, {1, 14}}}},
     // 8
     {{{194, 24, {2, 97}, {0, 0}}, {154, 22, {2, 38}, {2, 39}}, {110, 22, {4, 18}, {2, 19}}, {86, 26, {4, 14}, {2, 15}}}},
     // 9
     {{{232, 30, {2, 116}, {0, 0}}, {182, 22, {3, 36}, {2, 37}}, {132, 20, {4, 16}, {4, 17}}, {100, 24, {4, 12}, {4, 13}}}},
     // 10
     {{{274, 18, {2, 68}, {2, 69}}, {216, 26, {4, 43}, {1, 44}}, {154, 24, {6, 19}, {2, 20}}, {122, 28, {6, 15}, {2, 16}}}},
     // 11
     {{{324, 20, {4, 81}, {0, 0}}, {254, 30, {1, 50}, {4, 51}}, {180, 28, {4, 22}, {4, 23}}, {140, 24, {3, 12}, {8, 13}}}},
     // 12
     {{{370, 24, {2, 92}, {2, 93}}, {290, 22, {6, 36}, {2, 37}}, {206, 26, {4, 20}, {6, 21}}, {158, 28, {7, 14}, {4, 15}}}},
     // 13
     {{{428, 26, {4, 107}, {0, 0}}, {334, 22, {8, 37}, {1, 38}}, {244, 24, {8, 20}, {4, 21}}, {180, 22, {12, 11}, {4, 12}}}},
     // 14
     {{{461, 30, {3, 115}, {1, 116}}, {365, 24, {4, 40}, {5, 41}}, {261, 20, {11, 16}, {5, 17}}, {197, 24, {11, 12}, {5, 13}}}},
     // 15
     {{{523, 22, {5, 87}, {1, 88}}, {415, 24, {5, 41}, {5, 42}}, {295, 30, {5, 24}, {7, 25}}, {223, 24, {11, 12}, {7, 13}}}},
     // 16
     {{{589, 24, {5, 98}, {1, 99}}, {453, 28, {7, 45}, {3, 46}}, {325, 24, {15, 19}, {2, 20}}, {253, 30, {3, 15}, {13, 16}}}},
     // 17
     {{{647, 28, {1, 107}, {5, 108}}, {507, 28, {10, 46}, {1, 47}}, {367, 28, {1, 22}, {15, 23}}, {283, 28, {2, 14}, {17, 15}}}},
     // 18
     {{{721, 30, {5, 120}, {1, 121}}, {563, 26, {9, 43}, {4, 44}}, {397, 28, {17, 22}, {1, 23}}, {313, 28, {2, 14}, {19, 15}}}},
     // 19
     {{{795, 28, {3, 113}, {4, 114}}, {627, 26, {3, 44}, {11, 45}}, {445, 26, {17, 21}, {4, 22}}, {341, 26, {9, 13}, {16, 14}}}},
     // 20
     {{{861, 28, {3, 107}, {5, 108}}, {669, 26, {3, 41}, {13, 42}}, {485, 30, {15, 24}, {5, 25}}, {385, 28, {15, 15}, {10, 16}}}},
     // 21
     {{{932, 28, {4, 116}, {4, 117}}, {714, 26, {17, 42}, {0, 0}}, {512, 28, {17, 22}, {6, 23}}, {406, 30, {19, 16}, {6, 17}}}},
     // 22
     {{{1006, 28, {2, 111}, {7, 112}}, {782, 28, {17, 46}, {0, 0}}, {568, 30, {7, 24}, {16, 25}}, {442, 24, {34, 13}, {0, 0}}}},
     // 23
     {{{1094, 30, {4, 121}, {5, 122}}, {860, 28, {4, 47}, {14, 48}}, {614, 30, {11, 24}, {14, 25}}, {464, 30, {16, 15}, {14, 16}}}},
     // 24
     {{{1174, 30, {6, 117}, {4, 118}}, {914, 28, {6, 45}, {14, 46}}, {664, 30, {11, 24}, {16, 25}}, {514, 30, {30, 16}, {2, 17}}}},
     // 25
     {{{1276, 26, {8, 106}, {4, 107}}, {1000, 28, {8, 47}, {13, 48}}, {718, 30, {7, 24}, {22, 25}}, {538, 30, {22, 15}, {13, 16}}}},
     // 26
     {{{1370, 28, {10, 114}, {2, 115}}, {1062, 28, {19, 46}, {4, 47}}, {754, 28, {28, 22}, {6, 23}}, {596, 30, {33, 16}, {4, 17}}}},
     // 27
     {{{1468, 30, {8, 122}, {4, 123}}, {1128, 28, {22, 45}, {3, 46}}, {808, 30, {8, 23}, {26, 24}}, {628, 30, {12, 15}, {28, 16}}}},
     // 28
     {{{1531, 30, {3, 117}, {10, 118}}, {1193, 28, {3, 45}, {23, 46}}, {871, 30, {4, 24}, {31, 25}}, {661, 30, {11, 15}, {31, 16}}}},
     // 29
     {{{1631, 30, {7, 116}, {7, 117}}, {1267, 28, {21, 45}, {7, 46}}, {911, 30, {1, 23}, {37, 24}}, {701, 30, {19, 15}, {26, 16}}}},
     // 30
     {{{1735, 30, {5, 115}, {10, 116}}, {1373, 28, {19, 47}, {10, 48}}, {985, 30, {15, 24}, {25, 25}}, {745, 30, {23, 15}, {25, 16}}}},
     // 31
     {{{1843, 30, {13, 115}, {3, 116}}, {1455, 28, {2, 46}, {29, 47}}, {1033, 30, {42, 24}, {1, 25}}, {793, 30, {23, 15}, {28, 16}}}},
     // 32
     {{{1955, 30, {17, 115}, {0, 0}}, {1541, 28, {10, 46}, {23, 47}}, {1115, 30, {10, 24}, {35, 25}}, {845, 30, {19, 15}, {35, 16}}}},
     // 33
     {{{2071, 30, {17, 115}, {1, 116}}, {1631, 28, {14, 46}, {21, 47}}, {1171, 30, {29, 24}, {19, 25}}, {901, 30, {11, 15}, {46, 16}}}},
     // 34
     {{{2191, 30, {13, 115}, {6, 116}}, {1725, 28, {14, 46}, {23, 47}}, {1231, 30, {44, 24}, {7, 25}}, {961, 30, {59, 16}, {1, 17}}}},
     // 35
     {{{2306, 30, {12, 121}, {7, 122}}, {1812, 28, {12, 47}, {26, 48}}, {1286, 30, {39, 24}, {14, 25}}, {986, 30, {22, 15}, {41, 16}}}},
     // 36
     {{{2434, 30, {6, 121}, {14, 122}}, {1914, 28, {6, 47}, {34, 48}}, {1354, 30, {46, 24}, {10, 25}}, {1054, 30, {2, 15}, {64, 16}}}},
     // 37
     {{{2566, 30, {17, 122}, {4, 123}}, {1992, 28, {29, 46}, {14, 47}}, {1426, 30, {49, 24}, {10, 25}}, {1096, 30, {24, 15}, {46, 16}}}},
     // 38
     {{{2702, 30, {4, 122}, {18, 123}}, {2102, 28, {13, 46}, {32, 47}}, {1502, 30, {48, 24}, {14, 25}}, {1142, 30, {42, 15}, {32, 16}}}},
     // 39
     {{{2812, 30, {20, 117}, {4, 118}}, {2216, 28, {40, 47}, {7, 48}}, {1582, 30, {43, 24}, {22, 25}}, {1222, 30, {10, 15}, {67, 16}}}},
     // 40
     {{{2956, 30, {19, 118}, {6, 119}}, {2334, 28, {18, 47}, {31, 48}}, {1666, 30, {34, 24}, {34, 25}}, {1276, 30, {20, 15}, {61, 16}}}}}};

constexpr std::array<std::array<int, 4>, 3> CCI_BITS = {{
    {{10, 9, 8, 8}},    // Version 1–9
    {{12, 11, 16, 10}}, // Version 10–26
    {{14, 13, 16, 12}}  // Version 27–40
}};

const std::unordered_map<char, int> ALPHA_NUM_TABLE = {
    {'0', 0}, {'1', 1}, {'2', 2}, {'3', 3}, {'4', 4}, {'5', 5}, {'6', 6}, {'7', 7}, {'8', 8}, {'9', 9},

    {'A', 10},
    {'B', 11},
    {'C', 12},
    {'D', 13},
    {'E', 14},
    {'F', 15},
    {'G', 16},
    {'H', 17},
    {'I', 18},
    {'J', 19},
    {'K', 20},
    {'L', 21},
    {'M', 22},
    {'N', 23},
    {'O', 24},
    {'P', 25},
    {'Q', 26},
    {'R', 27},
    {'S', 28},
    {'T', 29},
    {'U', 30},
    {'V', 31},
    {'W', 32},
    {'X', 33},
    {'Y', 34},
    {'Z', 35},

    {' ', 36},
    {'$', 37},
    {'%', 38},
    {'*', 39},
    {'+', 40},
    {'-', 41},
    {'.', 42},
    {'/', 43},
    {':', 44}};

const int QR_CAPACITY[640] = {
    41, 25, 17, 10, 34, 20, 14, 8, 27, 16, 11, 7, 17, 10, 7, 4,
    77, 47, 32, 20, 63, 38, 26, 16, 48, 29, 20, 12, 34, 20, 14, 8,
    127, 77, 53, 32, 101, 61, 42, 26, 77, 47, 32, 20, 58, 35, 24, 15,
    187, 114, 78, 48, 149, 90, 62, 38, 111, 67, 46, 28, 82, 50, 34, 21,
    255, 154, 106, 65, 202, 122, 84, 52, 144, 87, 60, 37, 106, 64, 44, 27,
    322, 195, 134, 82, 255, 154, 106, 65, 178, 108, 74, 45, 139, 84, 58, 36,
    370, 224, 154, 95, 293, 178, 122, 75, 207, 125, 86, 53, 154, 93, 64, 39,
    461, 279, 192, 118, 365, 221, 152, 93, 259, 157, 108, 66, 202, 122, 84, 52,
    552, 335, 230, 141, 432, 262, 180, 111, 312, 189, 130, 80, 235, 143, 98, 60,
    652, 395, 271, 167, 513, 311, 213, 131, 364, 221, 151, 93, 288, 174, 119, 74,
    772, 468, 321, 198, 604, 366, 251, 155, 427, 259, 177, 109, 331, 200, 137, 85,
    883, 535, 367, 226, 691, 419, 287, 177, 489, 296, 203, 125, 374, 227, 155, 96,
    1022, 619, 425, 262, 796, 483, 331, 204, 580, 352, 241, 149, 427, 259, 177, 109,
    1101, 667, 458, 282, 871, 528, 362, 223, 621, 376, 258, 159, 468, 283, 194, 120,
    1250, 758, 520, 320, 991, 600, 412, 254, 703, 426, 292, 180, 530, 321, 220, 136,
    1408, 854, 586, 361, 1082, 656, 450, 277, 775, 470, 322, 198, 602, 365, 250, 154,
    1548, 938, 644, 397, 1212, 734, 504, 310, 876, 531, 364, 224, 674, 408, 280, 173,
    1725, 1046, 718, 442, 1346, 816, 560, 345, 948, 574, 394, 243, 746, 452, 310, 191,
    1903, 1153, 792, 488, 1500, 909, 624, 384, 1063, 644, 442, 272, 813, 493, 338, 208,
    2061, 1249, 858, 528, 1600, 970, 666, 410, 1159, 702, 482, 297, 919, 557, 382, 235,
    2232, 1352, 929, 572, 1708, 1035, 711, 438, 1224, 742, 509, 314, 969, 587, 403, 248,
    2409, 1460, 1003, 618, 1872, 1134, 779, 480, 1358, 823, 565, 348, 1056, 640, 439, 270,
    2620, 1588, 1091, 672, 2059, 1248, 857, 528, 1468, 890, 611, 376, 1108, 672, 461, 284,
    2812, 1704, 1171, 721, 2188, 1326, 911, 561, 1588, 963, 661, 407, 1228, 744, 511, 315,
    3057, 1853, 1273, 784, 2395, 1451, 997, 614, 1718, 1041, 715, 440, 1286, 779, 535, 330,
    3283, 1990, 1367, 842, 2544, 1542, 1059, 652, 1804, 1094, 751, 462, 1425, 864, 593, 365,
    3517, 2132, 1465, 902, 2701, 1637, 1125, 692, 1933, 1172, 805, 496, 1501, 910, 625, 385,
    3669, 2223, 1528, 940, 2857, 1732, 1190, 732, 2085, 1263, 868, 534, 1581, 958, 658, 405,
    3909, 2369, 1628, 1002, 3035, 1839, 1264, 778, 2181, 1322, 908, 559, 1677, 1016, 698, 430,
    4158, 2520, 1732, 1066, 3289, 1994, 1370, 843, 2358, 1429, 982, 604, 1782, 1080, 742, 457,
    4417, 2677, 1840, 1132, 3486, 2113, 1452, 894, 2473, 1499, 1030, 634, 1897, 1150, 790, 486,
    4686, 2840, 1952, 1201, 3693, 2238, 1538, 947, 2670, 1618, 1112, 684, 2022, 1226, 842, 518,
    4965, 3009, 2068, 1273, 3909, 2369, 1628, 1002, 2805, 1700, 1168, 719, 2157, 1307, 898, 553,
    5253, 3183, 2188, 1347, 4134, 2506, 1722, 1060, 2949, 1787, 1228, 756, 2301, 1394, 958, 590,
    5529, 3351, 2303, 1417, 4343, 2632, 1809, 1113, 3081, 1867, 1283, 790, 2361, 1431, 983, 605,
    5836, 3537, 2431, 1496, 4588, 2780, 1911, 1176, 3244, 1966, 1351, 832, 2524, 1530, 1051, 647,
    6153, 3729, 2563, 1577, 4775, 2894, 1989, 1224, 3417, 2071, 1423, 876, 2625, 1591, 1093, 673,
    6479, 3927, 2699, 1661, 5039, 3054, 2099, 1292, 3599, 2181, 1499, 923, 2735, 1658, 1139, 701,
    6743, 4087, 2809, 1729, 5313, 3220, 2213, 1362, 3791, 2298, 1579, 972, 2927, 1774, 1219, 750,
    7089, 4296, 2953, 1817, 5596, 3391, 2331, 1435, 3993, 2420, 1663, 1024, 3057, 1852, 1273, 784};

constexpr int ALIGNMENT_TABLE[41][7] = {
    {0}, // Not used
    {0}, // No alignment
    {6, 18},
    {6, 22},
    {6, 26},
    {6, 30},
    {6, 34},
    {6, 22, 38},
    {6, 24, 42},
    {6, 26, 46},
    {6, 28, 50},
    {6, 30, 54},
    {6, 32, 58},
    {6, 34, 62},
    {6, 26, 46, 66},
    {6, 26, 48, 70},
    {6, 26, 50, 74},
    {6, 30, 54, 78},
    {6, 30, 56, 82},
    {6, 30, 58, 86},
    {6, 34, 62, 90},
    {6, 28, 50, 72, 94},
    {6, 26, 50, 74, 98},
    {6, 30, 54, 78, 102},
    {6, 28, 54, 80, 106},
    {6, 32, 58, 84, 110},
    {6, 30, 58, 86, 114},
    {6, 34, 62, 90, 118},
    {6, 26, 50, 74, 98, 122},
    {6, 30, 54, 78, 102, 126},
    {6, 26, 52, 78, 104, 130},
    {6, 30, 56, 82, 108, 134},
    {6, 34, 60, 86, 112, 138},
    {6, 30, 58, 86, 114, 142},
    {6, 34, 62, 90, 118, 146},
    {6, 30, 54, 78, 102, 126, 150},
    {6, 24, 50, 76, 102, 128, 154},
    {6, 28, 54, 80, 106, 132, 158},
    {6, 32, 58, 84, 110, 136, 162},
    {6, 26, 54, 82, 110, 138, 166},
    {6, 30, 58, 86, 114, 142, 170}};
// |----------Constants----------|

// |----------Determine QR code information Start----------|
Encoding get_encoding(const std::string &input)
{
    // given an input string, determine its encoding mode
    // numeric, alphanumeric, byte mode (UTF-8), kanji
    bool isNumeric{true};
    bool isAlphaNumeric{true};
    bool isByteMode{true};
    bool isKanjiMode{true};

    const std::string alphanumeric = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

    for (int i = 0; i < input.size(); i++)
    {
        // numeric check
        unsigned char character = static_cast<unsigned char>(input[i]);
        if (!std::isdigit(character))
            isNumeric = false;

        // alphanumeric check
        if (alphanumeric.find(character) == std::string::npos)
            isAlphaNumeric = false;

        // kanji check (Shift-JIS 2-byte)
        if (i + 1 < input.size())
        {
            int value = (character << 8) | static_cast<unsigned char>(input[i + 1]);
            if ((value >= 0x8140 && value <= 0x9FFC) || (value >= 0xE040 && value <= 0xEBBF))
            {
                i += 1;
                continue;
            }
            else
                isKanjiMode = false;
        }
    }

    if (isNumeric)
        return Encoding::Numeric;
    if (isAlphaNumeric)
        return Encoding::Alphanumeric;
    if (isKanjiMode)
        return Encoding::Kanji;

    // since unsigned char is in range of 0 to 255, dont need to check for character is in this range or not
    return Encoding::Byte;
}

int get_smallest_version(const std::string &input, Encoding encoding, ECLevel errorCorrection)
{

    for (int version = 0; version < 40; version++)
    {
        // get the index for QR_CAPACITY based on the Encoding mode and Error Correction level given
        // Returns integer 1 -> 40 if suitable version is found, otherwise return 0 to indicate exceeded character limit
        int index{version * 16 + ((static_cast<int>(encoding) - 1) * 4) + static_cast<int>(errorCorrection)};
        if (input.size() <= QR_CAPACITY[index])
            return (version + 1);
    }
    return 0;
}
// |----------Determine QR code information End----------|

// |----------Shift JIS Start----------|
// pipeline: UTF-8 -> Unicode -> ShiftJIS
std::vector<uint32_t> utf8_convert_unicode(std::string input)
{
    // given a string that is in UTF-8 format, convert it into unicode string
    std::vector<uint32_t> result;

    size_t i = 0;
    while (i < input.size())
    {
        uint8_t byte = static_cast<uint8_t>(input[i]);

        uint32_t codepoint = 0;
        size_t length = 0;

        // Determine length (different length of bytes have different leading bits)
        // 0xxxxxxx - 1 byte, 110xxxxx - 2 byte, 1110xxxx - 3 byte, 11110xxx - 4 byte
        if ((byte & 0x80) == 0)
        {
            codepoint = byte;
            length = 1;
        }
        else if ((byte & 0xE0) == 0xC0)
        {
            codepoint = byte & 0x1F;
            length = 2;
        }
        else if ((byte & 0xF0) == 0xE0)
        {
            codepoint = byte & 0x0F;
            length = 3;
        }
        else if ((byte & 0xF8) == 0xF0)
        {
            codepoint = byte & 0x07;
            length = 4;
        }
        else
        {
            // invalid UTF-8
            i++;
            continue;
        }

        // Combine continuation bytes
        for (size_t j = 1; j < length; j++)
        {
            uint8_t next = static_cast<uint8_t>(input[i + j]);
            // shift the codepoint by 6 bits, then combine with next byte (removed leading 10 indicator)
            codepoint = (codepoint << 6) | (next & 0x3F);
        }

        // std::cout << std::hex << codepoint << '\n';
        result.push_back(codepoint);
        i += length;
    }
    return result;
}

std::vector<uint16_t> unicode_to_shiftJIS(std::vector<uint32_t> unicode)
{
    std::vector<uint16_t> shiftJIS;
    shiftJIS.reserve(unicode.size() * 2); // worst case

    for (uint32_t cp : unicode)
    {
        // If is ascii, no conversion is needed
        if (cp <= 0x7F)
        {
            shiftJIS.push_back(static_cast<uint8_t>(cp));
            continue;
        }

        // Half-width katakana (1 byte)
        if (cp >= 0xFF61 && cp <= 0xFF9F)
        {
            shiftJIS.push_back(static_cast<uint8_t>(cp - 0xFF61 + 0xA1));
            continue;
        }

        // Lookup in mapping table
        auto it = SHIFT_JIS_TABLE.find(cp);
        // Unable to find in lookup table
        if (it == SHIFT_JIS_TABLE.end())
        {
            // fallback for unsupported characters
            shiftJIS.push_back(0x3F); // '?'
            continue;
        }

        uint16_t sjis = it->second;
        // std::cout << std::hex << sjis << std::endl;
        // Convert to bytes
        shiftJIS.push_back(static_cast<uint16_t>(sjis));
    }

    return shiftJIS;
}

std::unordered_map<uint32_t, uint16_t> load_shiftJIS_table()
{
    // load file containing unicode -> shift JIS mappings into an unordered map
    std::unordered_map<uint32_t, uint16_t> shiftJISTable;
    std::ifstream shiftJISFile(SHIFT_JIS_FILE_PATH);
    std::string line;

    // load file and read lines
    while (std::getline(shiftJISFile, line))
    {
        uint32_t unicode{};
        uint16_t shiftJIS{};

        std::stringstream ss(line);
        ss >> std::hex >> shiftJIS >> unicode;

        // assign unicode -> shift JIS
        shiftJISTable[unicode] = shiftJIS;
    }

    return shiftJISTable;
}
// |----------Shift JIS End----------|

// |----------Data encoding Start----------|
std::string toBinary(int size)
{
    if (size == 0)
        return "0";

    std::string result;
    while (size > 0)
    {
        result.push_back((size & 1) ? '1' : '0'); // faster than %
        size >>= 1;
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::string get_cci(const std::string &input, QRinfo &qrinfo)
{
    int ccibits;
    // Get the number of required bits for the character count indictor
    if (qrinfo.version >= 1 && qrinfo.version <= 9)
        ccibits = CCI_BITS[0][static_cast<int>(qrinfo.encoding) - 1];
    else if (qrinfo.version >= 10 && qrinfo.version <= 26)
        ccibits = CCI_BITS[1][static_cast<int>(qrinfo.encoding) - 1];
    else
        ccibits = CCI_BITS[2][static_cast<int>(qrinfo.encoding) - 1];

    // Convert into character count of input into binary
    // Pad the binary so that it meets the minimum required bits
    std::string sizeBinary{toBinary(input.size())};
    sizeBinary.insert(0, ccibits - static_cast<int>(sizeBinary.size()), '0');

    return sizeBinary;
}

std::string get_terminator(uint16_t length, uint16_t requiredLength)
{
    if (length >= requiredLength)
        return "";

    uint16_t remaining = requiredLength - length;

    uint16_t terminatorLength = (remaining >= 4) ? 4 : remaining;

    return std::string(terminatorLength, '0');
}

std::string get_encoded(const std::string &input, QRinfo &qrinfo)
{
    std::string encoded{};
    // Encode the input based on its encoding mode
    if (qrinfo.encoding == Encoding::Numeric)
    {
        // Split string containing numeric into groups of 3 starting from the front
        for (int index = 0; index < input.size(); index += 3)
        {
            // Remove leading zeroes by converting to int
            int group{(std::stoi(input.substr(index, 3)))};

            int groupLength{static_cast<int>(std::to_string(group).size())};

            // Convert to binary bits with varying lengths based on group length
            // Length -> binary bits : 1->4, 2->7, 3->10
            if (groupLength == 1)
                encoded += std::bitset<4>(group).to_string();
            else if (groupLength == 2)
                encoded += std::bitset<7>(group).to_string();
            else if (groupLength == 3)
                encoded += std::bitset<10>(group).to_string();
        }
    }
    else if (qrinfo.encoding == Encoding::Alphanumeric)
    {
        for (int index = 0; index < input.size(); index += 2)
        {
            std::string pair(input.substr(index, 2));
            if (pair.size() == 2)
            {
                // For pairs: formula = index of first char * 45 + index of second char
                // Convert to 11 digit binary and pad with 0s
                encoded += std::bitset<11>(ALPHA_NUM_TABLE.at(pair[0]) * 45 + ALPHA_NUM_TABLE.at(pair[1])).to_string();
            }
            else if (pair.size() == 1)
                // For singles: formula = convert index of char to 6 digit binary and pad with 0s
                encoded += std::bitset<6>(ALPHA_NUM_TABLE.at(pair[0])).to_string();
        }
    }
    else if (qrinfo.encoding == Encoding::Byte)
    {
        for (auto character : input)
        {
            encoded += std::bitset<8>(character).to_string();
        }
    }
    else if (qrinfo.encoding == Encoding::Kanji)
    {
        // Convert UTF-8 to Unicode then to ShiftJIS before encoding
        for (uint16_t character : unicode_to_shiftJIS(utf8_convert_unicode(input)))
        {
            uint16_t subtracted{};
            // For characters whose bytes are in the range 0x8140 to 0x9FFC:
            // Substract 0x8140 from hex value
            if (character >= 0x8140 && character <= 0x9FFC)
                subtracted = static_cast<uint16_t>(character - 0x8140);

            // For characters whose bytes are in the range 0xE040 to 0xEBBF
            // Substract 0xC140 from hex value
            else if (character >= 0xE040 && character <= 0xEBBF)
                subtracted = static_cast<uint16_t>(character - 0xC140);

            // Split that substracted number into its most significant byte and its least significant byte
            uint16_t highByte{static_cast<uint16_t>((subtracted >> 8) & 0xFF)};
            uint16_t lowByte{static_cast<uint16_t>(subtracted & 0xFF)};

            // Multiply the most significant byte by 0xC0, then add the least significant byte to the result
            // Convert result into 13-bit binary
            encoded += (std::bitset<13>((highByte * 0xC0) + lowByte)).to_string();

            // std::cout << std::hex << (std::bitset<13>((highByte * 0xC0) + lowByte)).to_string() << std::endl;
        }
    }
    // Get the mode indicator
    // Numeric - 0001, Alphanumeric - 0010, Byte - 0100, Kanji - 1000
    std::string modeIndicator{(std::bitset<4>(1 << (static_cast<int>(qrinfo.encoding) - 1))).to_string()};

    // Get the character count indicator
    std::string cci{get_cci(input, qrinfo)};

    // Character indicator count is inserted between mode indicator and encoded string

    // Size of current encoded bits
    uint16_t length{static_cast<uint16_t>(modeIndicator.size() + cci.size() + encoded.size())};

    // Get the required number of bits
    uint16_t requiredLength{static_cast<uint16_t>(EC_TABLE[qrinfo.version][static_cast<int>(qrinfo.eclevel)].totalDataCodewords * 8)};

    std::string terminator{get_terminator(length, requiredLength)};

    // Add mode indicator, cci and terminator to encoded
    encoded.insert(0, cci);
    encoded.insert(0, modeIndicator);
    encoded += terminator;

    // If zeroes to make length a multiple of 8
    int remainder = encoded.size() % 8;
    if (remainder != 0)
        encoded += std::string(8 - remainder, '0');

    // Add pad bytes if does not meet required length
    uint16_t remainingBits = requiredLength - encoded.size();

    uint16_t remainingBytes = remainingBits / 8;

    for (uint16_t index = 1; index <= remainingBytes; index++)
    {
        if (index % 2 == 1)
            encoded += "11101100";
        else
            encoded += "00010001";
    }

    std::cout << encoded << std::endl;
    return encoded;
}
// |----------Data Encoding End----------|

// |----------Error Correction Start----------|
constexpr std::array<uint8_t, 256> build_EXP()
{
    std::array<uint8_t, 256> exp;

    exp[0] = 1;
    for (int index = 1; index <= 255; index++)
    {
        int integer{exp[index - 1] * 2};

        // If integer is greater than 255, XOR the integer with 285
        exp[index] = (integer > 255) ? (integer ^ 285) : integer;
    }
    return exp;
}

constexpr std::array<uint8_t, 256> build_LOG()
{
    std::array<uint8_t, 256> log;

    int previous{1};
    for (int index = 1; index <= 255; index++)
    {
        int integer{previous * 2};

        previous = (integer > 255) ? (integer ^ 285) : integer;

        if (previous != 0)
            log[previous] = index % 255;
    }
    // cannot log 0 thus empty
    log[0] = {};

    return log;
}

std::vector<uint8_t> multiply_poly(std::vector<uint8_t> poly1, std::vector<uint8_t> poly2)
{
    // Number of coefficients will always be (number of terms of poly 1 + number of terms of poly 2 - 1)
    // NOTE : Polynomials are initially in alpha notation
    std::vector<uint8_t> coefficients(poly1.size() + poly2.size() - 1, 0);

    for (size_t index = 0; index < coefficients.size(); index++)
    {
        uint64_t i{coefficients.size() - index - 1};
        for (size_t i1 = 0; i1 < poly1.size(); i1++)
        {
            if (index < i1)
                continue;

            uint8_t i2{static_cast<uint8_t>(index - i1)};

            if (i2 >= poly2.size())
                continue;

            // Add up the indices from each polynomial then antiLOG the result
            // In this degree, XOR the previous result / 0 with the new antiLOG
            coefficients[index] ^= (EXP[(poly1[i1] + poly2[i2]) % 255]);
        }

        // Convert back to alpha notation
        coefficients[index] = LOG[coefficients[index]];
    }

    return coefficients;
}

std::vector<uint8_t> build_message_poly(const std::string &encoded)
{
    // Split encoded message into groups of 8 bits
    // Convert the characters into binary
    uint64_t nGroups{(encoded.size() / 8)};
    std::vector<uint8_t> messagePoly(nGroups);

    for (int index = 0; index < nGroups; index++)
    {
        uint8_t value = 0;
        for (int i = 0; i < 8; ++i)
        {
            value = (value << 1) | (encoded[index * 8 + i] - '0');
        }
        messagePoly[index] = value;
    }
    return messagePoly;
}

std::vector<uint8_t> build_generator_poly(int degree)
{
    std::vector<uint8_t> result{0, 0};

    for (uint8_t d = 1; d < degree; d++)
    {
        // For every increasing degree, multiply previous by (x - a^d)
        std::vector<uint8_t> newPoly{0, d};
        // Reassign result to the multiplied polynomial
        result = multiply_poly(result, newPoly);
    }
    return result;
}

std::vector<uint8_t> get_remainder(std::vector<uint8_t> messagePoly, std::vector<uint8_t> generatorPoly, int numberEC)
{
    // Carry out long division until remainder is same as the number of EC codewords per block
    // Remainder will be the Error Correction codeword

    std::vector<uint8_t> buffer = messagePoly;
    buffer.resize(messagePoly.size() + numberEC, 0);

    size_t dataLen = messagePoly.size();
    size_t genSize = generatorPoly.size();
    // NOTE : messagePoly is in normal form while generatorPoly is in alpha notation

    for (size_t i = 0; i < dataLen; i++)
    {
        uint8_t lead = buffer[i];

        if (lead != 0)
        {
            uint8_t logLead = LOG[lead];

            for (size_t j = 0; j < genSize; j++)
            {
                uint8_t genExp = generatorPoly[j];

                // Convert the sum of coefficient of generator polynomial with multiplier mod 255 into normal integer
                // XOR the result with previous result's coefficient
                uint8_t product = EXP[(logLead + genExp) % 255];

                // NOTE: If run out of terms, XOR result with 0
                buffer[i + j] ^= product;
            }
        }
    }
    // return remainder in integer notation form
    return std::vector<uint8_t>(
        buffer.end() - numberEC,
        buffer.end());
}

std::vector<std::vector<uint8_t>> get_dc_blocks(std::string encoded, QRinfo &qrinfo)
{
    std::vector<std::vector<uint8_t>> dcBlocks;

    BlockGroup g1 = EC_TABLE[qrinfo.version][static_cast<int>(qrinfo.eclevel)].g1;
    BlockGroup g2 = EC_TABLE[qrinfo.version][static_cast<int>(qrinfo.eclevel)].g2;

    // Split the encoded string into 1 or 2 blocks with n codewords based on its encoding mode, version and EC level
    for (int nBlock = 0; nBlock < g1.blocks; nBlock++)
    {
        dcBlocks.push_back(build_message_poly(encoded.substr(nBlock * g1.codewords * 8, g1.codewords * 8)));
    }

    int covered{g1.blocks * g1.codewords * 8};
    for (int nBlock = 0; nBlock < g2.blocks; nBlock++)
    {
        dcBlocks.push_back(build_message_poly(encoded.substr(covered + (nBlock * g2.codewords * 8), g2.codewords * 8)));
    }

    return dcBlocks;
}

std::vector<std::vector<uint8_t>> get_ecc_blocks(std::vector<std::vector<uint8_t>> &dcBlocks, QRinfo &qrinfo)
{
    // Generate EC codeword blocks given the data codewords block
    std::vector<std::vector<uint8_t>> eccBlocks;

    // Get the generator polynomial
    std::vector<uint8_t> generatorPoly{build_generator_poly(EC_TABLE[qrinfo.version][static_cast<int>(qrinfo.eclevel)].ecCodewordsPerBlock)};

    for (auto block : dcBlocks)
    {
        eccBlocks.push_back(get_remainder(block, generatorPoly, EC_TABLE[qrinfo.version][static_cast<int>(qrinfo.eclevel)].ecCodewordsPerBlock));
    }

    return eccBlocks;
}
// |----------Error Correction End----------|

void interleave_block(std::vector<std::vector<uint8_t>> &blocks, std::string &structured)
{
    uint64_t maxIndex{(blocks[0].size() > blocks.back().size()) ? blocks[0].size() : blocks.back().size()};

    for (int i = 0; i < maxIndex; i++)
    {
        // iterate through the blocks
        for (int j = 0; j < blocks.size(); j++)
        {
            if (i < blocks[j].size())
                structured += (std::bitset<8>(blocks[j][i])).to_string();
        }
    }
}

std::string interleave(std::vector<std::vector<uint8_t>> &dcBlocks, std::vector<std::vector<uint8_t>> &eccBlocks)
{
    std::string structured;
    // interleave for data codewords blocks
    interleave_block(dcBlocks, structured);
    // interleave for error correction codewords blocks
    interleave_block(eccBlocks, structured);

    return structured;
}

// |----------QR code matrix generation Start----------|
void finder_seperator(int x, int y, std::vector<std::vector<int8_t>> &qrCode, int size)
{
    // Place separator (8x8 area, outer border = 0)
    for (int r = -1; r <= 7; r++)
    {
        for (int c = -1; c <= 7; c++)
        {
            int rr = x + r;
            int cc = y + c;

            if (rr < 0 || cc < 0 || rr >= size || cc >= size)
                continue;

            // Only write separator if outside the 7x7 finder
            if (r == -1 || r == 7 || c == -1 || c == 7)
            {
                qrCode[rr][cc] = 0;
            }
        }
    }

    // Given the x and y coordinate of the top left corner of finder pattern
    // Fill in the finder pattern (7 x 7)
    for (int i = 0; i < 7; i++)
    {
        for (int j = 0; j < 7; j++)
        {
            qrCode[x + i][y + j] = finder[i][j];
        }
    }
}

void place_alignment_unit(int x, int y, std::vector<std::vector<int8_t>> &qrCode)
{
    // Place 5 x 5 alignment pattern given its centre coordinate
    for (int i = -2; i <= 2; i++)
    {
        for (int j = -2; j <= 2; j++)
        {
            qrCode[x + i][y + j] = alignment[i + 2][j + 2];
        }
    }
}

void fill_alignment_patterns(std::vector<std::vector<int8_t>> &qrCode, int version)
{
    // The coordinates are predefined, thus only fill coordinates which do not overlap with finder / separator
    for (auto xCoord : ALIGNMENT_TABLE[version])
    {
        if (xCoord == 0)
            continue;

        for (auto yCoord : ALIGNMENT_TABLE[version])
        {
            if (yCoord == 0)
                continue;

            if (qrCode[xCoord][yCoord] == 0 || qrCode[xCoord][yCoord] == 1)
                continue;
            else
                place_alignment_unit(xCoord, yCoord, qrCode);
        }
    }
}

void fill_timing_patterns(std::vector<std::vector<int8_t>> &qrCode, int size)
{
    // Always start from (6, 8) and (8, 6) index
    for (int i = 0; i < size - 16; i++)
    {
        // Timing pattern always start and ends with black
        // Able to overlap with alignment patterns as the white and black modules coincide

        // Fill in horizontal timing pattern
        qrCode[6][8 + i] = (i % 2) ? 0 : 1;
        // Fill in vertical timing pattern
        qrCode[8 + i][6] = (i % 2) ? 0 : 1;
    }
}

void fill_reserved_area(std::vector<std::vector<int8_t>> &qrCode, int version)
{
    int size = qrCode.size();
    for (int i = 0; i < 8; i++)
    {
        // Fill reserved area near top left finder pattern
        if (qrCode[8][i] == -1)
            qrCode[8][i] = 0;
        if (qrCode[i][8] == -1)
            qrCode[i][8] = 0;

        qrCode[8][8] = 0;

        // Fill reserved area near top right finder pattern
        if (qrCode[8][size - (i + 1)] == -1)
            qrCode[8][size - (i + 1)] = 0;

        // Fill reserved area near bottom left finder pattern
        if (qrCode[size - (i + 1)][8] == -1)
            qrCode[size - (i + 1)][8] = 0;
    }

    // For versions 7 or larger, version information bits are placed within the 2 following reserved areas
    if (version >= 7)
    {
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 6; j++)
            {
                // Reserve an area of 6 x 3 block above bottom left finder
                qrCode[j][size - (i + 9)] = 0;
                // Reserve an area of 3 x 6 block to the right of top right finder
                qrCode[size - (i + 9)][j] = 0;
            }
        }
    }
}

void fill_data_bits(std::vector<std::vector<int8_t>> &qrCode, std::string &finalEncoded)
{
    std::vector<std::vector<std::vector<int8_t>>> masked(8, qrCode);

    int tracker{0};
    int size = qrCode.size();

    // Each "zigzag" represents a pair of columns (moving right → left)
    for (int zigzag = 0, col = size - 1; col > 0; zigzag++, col -= 2)
    {
        // Skip the vertical timing pattern column (column 6)
        if (col == 6)
            col--;

        // Determine direction:
        // even zigzag → upwards (-1)
        // odd zigzag  → downwards (+1)
        int direction = (zigzag % 2 == 0) ? -1 : 1;

        // Starting row depends on direction
        int row = (direction == -1) ? size - 1 : 0;

        // Traverse entire column pair
        for (int step = 0; step < size; step++)
        {
            // Each zigzag column is 2 columns wide
            for (int i = 0; i < 2; i++)
            {
                int currentCol = col - i; // right column first, then left

                // Only fill if module is empty (-1 = not reserved / not filled)
                if (qrCode[row][currentCol] == -1)
                {
                    qrCode[row][currentCol] = finalEncoded[tracker] - '0';
                    tracker++;
                }
            }

            // Move up or down depending on zigzag direction
            row += direction;
        }
    }
}

void get_mask(std::vector<std::vector<int8_t>> &copy, std::vector<std::vector<int8_t>> &bitmask, int maskVer)
{
    int size{static_cast<int>(copy.size())};

    for (int row = 0; row < size; row++)
    {
        for (int col = 0; col < size; col++)
        {
            if (bitmask[row][col] == -1)
            {
                int value{copy[row][col]};

                switch (maskVer)
                {
                case 0:
                    copy[row][col] = ((row + col) % 2 == 0) ? (value ^ 1) : value;
                    break;

                case 1:
                    copy[row][col] = (row % 2 == 0) ? (value ^ 1) : value;
                    break;

                case 2:
                    copy[row][col] = (col % 3 == 0) ? (value ^ 1) : value;
                    break;

                case 3:
                    copy[row][col] = ((row + col) % 3 == 0) ? (value ^ 1) : value;
                    break;

                case 4:
                    copy[row][col] = (((row / 2) + (col / 3)) % 2 == 0) ? (value ^ 1) : value;
                    break;

                case 5:
                    copy[row][col] = (((row * col) % 2 + (row * col) % 3) == 0) ? (value ^ 1) : value;
                    break;

                case 6:
                    copy[row][col] = ((((row * col) % 2 + (row * col) % 3) % 2) == 0) ? (value ^ 1) : value;
                    break;

                case 7:
                    copy[row][col] = ((((row + col) % 2 + (row * col) % 3) % 2) == 0) ? (value ^ 1) : value;
                    break;
                }
            }
        }
    }
}

int calculate_penalty(std::vector<std::vector<int8_t>> &copy)
{
    int penalty{0};
    int size{static_cast<int>(copy.size())};
    // |----------Condition 1----------|
    // Iterate through rows and columns, if there are five consecutive module of same color,
    // give 3 penalty points and add 1 for any additional module of same color thereafter

    for (int i = 0; i < size; i++)
    {
        // Horizontal
        int countH{1};
        int previousH{copy[i][0]};

        // Vertical
        int countV{1};
        int previousV{copy[0][i]};

        for (int j = 1; j < size; j++)
        {
            // Horizontal
            if (copy[i][j] == previousH)
                countH += 1;

            // If not consecutive, check count then reset both the count and previous
            else
            {
                // Five consecutive
                if (countH >= 5)
                    penalty += (countH - 2);

                countH = 1;
                previousH = copy[i][j];
            }

            // vertical
            if (copy[j][i] == previousV)
                countV += 1;

            // If not consecutive, check count then reset both the count and previous
            else
            {
                // Five consecutive
                if (countV >= 5)
                    penalty += (countV - 2);

                countV = 1;
                previousV = copy[j][i];
            }
        }

        // After finishing row i
        if (countH >= 5)
            penalty += (countH - 2);

        // After finishing column i
        if (countV >= 5)
            penalty += (countV - 2);
    }

    // |----------Condition 2----------|
    // Starting from (0,0), count the number of 2 x 2 squares of same color
    // For each square, add 3 penalty points
    for (int i = 0; i < size - 1; i++)
    {
        for (int j = 0; j < size - 1; j++)
        {
            // If all modules in 2 x 2 are same colour
            if (copy[i][j] == copy[i + 1][j] && copy[i][j] == copy[i][j + 1] && copy[i][j] == copy[i + 1][j + 1])
            {
                penalty += 3;
            }
        }
    }

    // |----------Condition 3----------|
    // Penalise patterns of dark - light - dark - dark - dark - light - dark that have 4 light on either side
    // Eg: 00001011101 / 10111010000 (horizontally and vertically)
    // Anchor to the first module of the 1011101
    std::array<int, 7> pattern{{1, 0, 1, 1, 1, 0, 1}};
    // Check for 1011101 pattern horizontally
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j <= size - 7; j++)
        {
            // 1. Check 1011101
            bool match = true;
            for (int k = 0; k < 7; k++)
            {
                if (copy[i][j + k] != pattern[k])
                {
                    match = false;
                    break;
                }
            }

            if (!match)
                continue;

            // 2. Check LEFT (0000 1011101)
            if (j >= 4 &&
                copy[i][j - 1] == 0 &&
                copy[i][j - 2] == 0 &&
                copy[i][j - 3] == 0 &&
                copy[i][j - 4] == 0)
            {
                penalty += 40;
            }

            // 3. Check RIGHT (1011101 0000)
            if (j + 7 <= size - 4 &&
                copy[i][j + 7] == 0 &&
                copy[i][j + 8] == 0 &&
                copy[i][j + 9] == 0 &&
                copy[i][j + 10] == 0)
            {
                penalty += 40;
            }
        }
    }

    // Check for 1011101 pattern vertically
    for (int j = 0; j < size; j++)
    {
        for (int i = 0; i <= size - 7; i++)
        {
            bool match = true;
            for (int k = 0; k < 7; k++)
            {
                if (copy[i + k][j] != pattern[k])
                {
                    match = false;
                    break;
                }
            }

            if (!match)
                continue;

            // UP
            if (i >= 4 &&
                copy[i - 1][j] == 0 &&
                copy[i - 2][j] == 0 &&
                copy[i - 3][j] == 0 &&
                copy[i - 4][j] == 0)
            {
                penalty += 40;
            }

            // DOWN
            if (i + 7 <= size - 4 &&
                copy[i + 7][j] == 0 &&
                copy[i + 8][j] == 0 &&
                copy[i + 9][j] == 0 &&
                copy[i + 10][j] == 0)
            {
                penalty += 40;
            }
        }
    }

    // |----------Condition 4----------|
    // Ratio of light to dark modules
    int countDark{0};
    for (int i = 0; i < size; i++)
    {
        for (int j = 0; j < size; j++)
        {
            if (copy[i][j] == 1)
                countDark++;
        }
    }

    // Compute percentage of dark modules
    // Use floating point to avoid integer division truncation
    double percentage = (double)countDark * 100.0 / (size * size);

    // Calculate how far the percentage is from the ideal 50%
    double diff = std::abs(percentage - 50.0);

    // Each 5% deviation from 50% adds a penalty step
    // (e.g. 55% → 1 step, 62% → 2 steps, etc.)
    int steps = (int)(diff / 5.0);

    // Each step contributes 10 penalty points
    penalty += steps * 10;

    return penalty;
}

int get_best_mask(std::vector<std::vector<int8_t>> &qrCode, std::vector<std::vector<int8_t>> &bitmask, bool printMask = false)
{
    int bestMask = 0;
    int bestScore = INT_MAX;

    for (int maskVer = 0; maskVer < 8; maskVer++)
    {
        // Make a copy of the base unmasked QR Code
        auto temp = qrCode;

        // Apply mask
        get_mask(temp, bitmask, maskVer);

        // Print all masks
        if (true)
            print_qr(temp, true);
        // Calculate the penalty score
        int score = calculate_penalty(temp);

        // Keep the best one
        if (score < bestScore)
        {
            bestScore = score;
            bestMask = maskVer;
        }
    }
    return bestMask;
}

uint16_t getFormatString(QRinfo &qrinfo, int bestMask)
{
    std::string ecString;
    switch (qrinfo.eclevel)
    {
    case (ECLevel::L):
        ecString = "01";
        break;
    case (ECLevel::M):
        ecString = "00";
        break;
    case (ECLevel::Q):
        ecString = "11";
        break;
    case (ECLevel::H):
        ecString = "10";
        break;
    }
    // Get the error correction bits
    uint16_t temp{static_cast<uint16_t>(std::stoi(ecString + std::bitset<3>(bestMask).to_string(), nullptr, 2))};

    uint16_t padded{static_cast<uint16_t>(temp << (10))};
    uint16_t generator{0b10100110111};

    uint16_t remainder{padded};
    // Step 3: BCH division
    for (int i = 14; i >= 10; i--)
    {
        if ((remainder >> i) & 1)
        {
            remainder ^= (generator << (i - 10));
        }
    }

    // Combine padded with error correction bits
    uint16_t formatBits = padded | (remainder & 0x3FF);

    // Apply mask
    formatBits ^= 0b101010000010010;

    return formatBits;
}

uint32_t getVersionInfoBits(int version)
{
    // Version is 6 bits
    uint32_t data = static_cast<uint32_t>(version) & 0x3F;

    // Shift left 12 bits (for BCH remainder space)
    data <<= 12;

    uint32_t poly = 0x1F25; // generator polynomial

    // We operate on 18-bit space
    for (int i = 17; i >= 12; --i)
    {
        if (data & (1 << i))
        {
            data ^= (poly << (i - 12));
        }
    }

    // Final 18-bit version info
    uint32_t versionInfo = ((version & 0x3F) << 12) | (data & 0xFFF);

    return versionInfo;
}

void fill_format_string(std::vector<std::vector<int8_t>> &qrCode, uint16_t formatString)
{
    int tracker{0};
    auto size{qrCode.size()};
    std::string fs{(std::bitset<15>(formatString)).to_string()};

    // Fill in format string bits below top left finder pattern
    for (int i = 0; i < 9; i++)
    {
        // Skip the timing pattern module
        if (i != 6)
        {
            qrCode[8][i] = fs[tracker] - '0';
            tracker++;
        }
    }

    // Fill in format string bits to the right of top left finder pattern
    for (int i = 7; i >= 0; i--)
    {
        // Skip the timing pattern module
        if (i != 6)
        {
            qrCode[i][8] = fs[tracker] - '0';
            tracker++;
        }
    }

    // Reset tracker
    tracker = 0;
    // Fill in format string bits at the right of bottom left finder pattern
    for (int i = 0; i < 7; i++)
    {
        qrCode[size - 1 - i][8] = fs[tracker] - '0';
        tracker++;
    }

    // Fill in format string bits at the bottom of top right finder pattern
    for (int i = 7; i >= 0; i--)
    {
        qrCode[8][size - 1 - i] = fs[tracker] - '0';
        tracker++;
    }
}

void fill_version_info(std::vector<std::vector<int8_t>> &qrCode, uint32_t versionInfoBits)
{
    int tracker{0};
    auto size(qrCode.size());

    std::string versionInfo{(std::bitset<18>(versionInfoBits).to_string())};

    // Fill in version info bits above the bottom left finder pattern
    for (int i = 0; i <= 5; i++)
    {
        for (int j = 11; j >= 9; j--)
        {
            qrCode[size - j][i] = versionInfo[17 - tracker] - '0';
            tracker++;
        }
    }
    // Reset finder pattern
    tracker = 0;

    // Fill in version info bits to the left of the top right finder pattern
    for (int i = 0; i <= 5; i++)
    {
        for (int j = 11; j >= 9; j--)
        {
            qrCode[i][size - j] = versionInfo[17 - tracker] - '0';
            tracker++;
        }
    }
}

std::vector<std::vector<int8_t>> build_qr(std::string &finalEncoded, QRinfo qrinfo)
{
    // Given a processed encoded message, convert it into qr code

    int qrSize{((qrinfo.version - 1) * 4) + 21};

    // Initialise blank qr code of n size
    std::vector<std::vector<int8_t>> qrCode(qrSize, std::vector<int8_t>(qrSize, -1));

    // Fill in top left finder pattern and separator
    finder_seperator(0, 0, qrCode, qrSize);
    // Fill in top right finder pattern and separator
    finder_seperator(qrSize - 7, 0, qrCode, qrSize);
    // Fill in bottom left finder pattern and separator
    finder_seperator(0, qrSize - 7, qrCode, qrSize);

    // Fill in alignment patterns
    fill_alignment_patterns(qrCode, qrinfo.version);

    print_qr(qrCode, false);

    // Fill in timing patterns
    fill_timing_patterns(qrCode, qrSize);

    // Fill in the dark module
    qrCode[(4 * qrinfo.version) + 9][8] = 1;

    // Fill in reserved areas
    fill_reserved_area(qrCode, qrinfo.version);

    // Matrix to indicate which cell to be masked
    std::vector<std::vector<int8_t>> bitmask{qrCode};

    // Fill in databits
    fill_data_bits(qrCode, finalEncoded);

    int bestMask{get_best_mask(qrCode, bitmask)};

    // Apply best mask on qrCode
    get_mask(qrCode, bitmask, bestMask);

    // Fill in format string
    fill_format_string(qrCode, getFormatString(qrinfo, bestMask));

    // Fill in version info for qr code that are version 7 or higher
    if (qrinfo.version >= 7)
        fill_version_info(qrCode, getVersionInfoBits(qrinfo.version));

    return qrCode;
}
// |----------QR code matrix generation End----------|

std::expected<std::vector<std::vector<int8_t>>, ErrorType> generate_qr(const std::string &input,
                                                                       ECLevel errorCorrection = ECLevel::L,
                                                                       std::optional<int> ver = std::nullopt,
                                                                       std::optional<Encoding> encoding = std::nullopt)
{
    // |----------Error Checking start----------|
    // Given an input string, generate the QR code

    // Get the most suitable encoding mode
    // If the encoding mode given by user is unable to encode the input fully, return error
    Encoding optimalEncoding{get_encoding(input)};
    if (encoding.has_value())
    {
        if (encoding.value() < optimalEncoding)
            return std::unexpected(ErrorType::IncorrectEncodingModeGiven);
    }
    else
        encoding = optimalEncoding;

    // Check whether the version given is valid
    if (ver.has_value())
    {
        if (ver.value() <= 0 || ver.value() > 40)
            return std::unexpected(ErrorType::InvalidVersionGiven);
    }

    // Get the smallest version possible to input given encoding mode and error correction level
    int version{get_smallest_version(input, encoding.value(), errorCorrection)};

    // size limit exceeded
    if (version == 0)
        return std::unexpected(ErrorType::CharacterLimitExceeded);

    // If choosen version is greater than the minimum version required, use the choosen version instead
    if (ver.value() >= version)
        version = ver.value();

    // |----------Error Checking End----------|

    // |----------Encoding Start----------|
    QRinfo qrinfo{version, encoding.value(), errorCorrection};

    // Convert input into encoded string
    std::string encoded{get_encoded(input, qrinfo)};

    // Get the data codewords block (in binary)
    std::vector<std::vector<uint8_t>> dcBlocks{get_dc_blocks(encoded, qrinfo)};

    // Get the ECC block given the dcBlocks
    std::vector<std::vector<uint8_t>> eccBlocks{get_ecc_blocks(dcBlocks, qrinfo)};

    // Interleave dc blocks and ecc blocks
    std::string result{interleave(dcBlocks, eccBlocks)};

    // Add padding according to version
    result.append(VERSIONPADDING[qrinfo.version - 1], '0');
    std::cout << result << '\n';

    // |----------Encoding End----------|

    // Module placement
    // Return the qrcode
    return build_qr(result, qrinfo);
}

void print_qr(std::vector<std::vector<int8_t>> &qrcode, bool padding)
{
    // If padding mode, add padding to edges of qr code

    if (padding)
    {
        for (int row = 0; row < 3; row++)
        {
            for (int i = 0; i < (qrcode.size() + 8); i++)
            {
                std::cout << "██";
            }
            std::cout << std::endl;
        }
    }

    // Print out the given qrcode content
    for (auto row : qrcode)
    {
        if (padding)
        {
            for (int i = 0; i < 4; i++)
            {
                std::cout << "██";
            }
        }
        for (auto module : row)
        {
            if (module == -1)
                std::cout << "  "; // empty
            else if (module == 0)
                std::cout << "██"; // white
            else
                std::cout << "  "; // black
        }
        if (padding)
        {
            for (int i = 0; i < 4; i++)
            {
                std::cout << "██";
            }
        }
        std::cout << std::endl;
    }

    if (padding)
    {
        for (int row = 0; row < 3; row++)
        {
            for (int i = 0; i < (qrcode.size() + 8); i++)
            {
                std::cout << "██";
            }
            std::cout << std::endl;
        }
    }
}

void print_alpha(std::vector<uint8_t> poly)
{
    // Print out a polynomial in its alpha notation
    for (int degree = 0; degree < poly.size(); degree++)
    {
        std::cout << "a^" << (int)poly[degree] << "x^" << poly.size() - degree - 1;

        if (degree != (poly.size() - 1))
            std::cout << " + ";
        else
            std::cout << '\n';
    }
}

int main()
{
    // set console input and output as UTF-8 for windows
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    while (true)
    {
        // prompt user to input the text to be converted to QR
        std::string input;
        std::cout << "Input text : ";
        getline(std::cin, input);

        // Prompt user for the error correction level
        std::string ecLevel;
        while (!(ecLevel == "L" || ecLevel == "M" || ecLevel == "Q" || ecLevel == "H"))
        {
            std::cout << "Error correction level (L, M, Q, H) : ";
            getline(std::cin, ecLevel);
        }

        ECLevel ec{};
        if (ecLevel == "L")
            ec = ECLevel::L;
        else if (ecLevel == "M")
            ec = ECLevel::M;
        else if (ecLevel == "Q")
            ec = ECLevel::Q;
        else if (ecLevel == "H")
            ec = ECLevel::H;

        auto qrCode{generate_qr(input, ec, 10)};

        if (qrCode)
            print_qr(qrCode.value(), true);
        else
            std::cout << "Unable to generate qr code" << std::endl;

        // Prompt user whether want to continue generating QR Code
        std::string continueFlag;
        while (!(continueFlag == "Y" || continueFlag == "N"))
        {
            std::cout << "Continue generating QR Code? (Y/N) ";
            getline(std::cin, continueFlag);
        }

        if (continueFlag == "N")
            break;
        else
            std::cout << "\033[2J\033[H";
    }
}
