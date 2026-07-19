#include "Autotile.h"

namespace {

using namespace autotile;

// Canonical neighbor mask of each Case (bit set = neighbor of the SAME
// terrain). Corner bits only count when both adjacent cardinal bits are set.
// Index order must match the Case enum.
constexpr Uint8 CASE_MASK[CaseCount] = {
    255,              // Center
    110, 205, 155, 55,   // EdgeN EdgeE EdgeS EdgeW
    38, 76, 137, 19,     // CornerNW NE SE SW
    10, 5,               // HStripM VStripM
    2, 8, 4, 1,          // HStripL HStripR VCapT VCapB
    0,                   // Iso
    127, 239, 223, 191,  // InNW InNE InSE InSW
    111, 207, 159, 63,   // InNW_NE InNE_SE InSE_SW InSW_NW
    95, 175,             // InNW_SE InNE_SW
    79, 47, 31, 143,     // InNW_NE_SE InNW_NE_SW InNW_SE_SW InNE_SE_SW
    15,                  // InAll
    46, 78, 14,          // EdgeN_InSW _InSE _InSW_SE
    77, 141, 13,         // EdgeE_InNW _InSW _InNW_SW
    27, 139, 11,         // EdgeS_InNW _InNE _InNW_NE
    39, 23, 7,           // EdgeW_InNE _InSE _InNE_SE
    6, 12, 9, 3,         // CornerNW_InSE CornerNE_InSW CornerSE_InNW CornerSW_InNE
};

// Drops corner bits whose adjacent cardinal bits are not both set.
Uint8 canon(Uint8 m) {
    if (!(m & N) || !(m & E)) m &= ~NE;
    if (!(m & E) || !(m & S)) m &= ~SE;
    if (!(m & S) || !(m & W)) m &= ~SW;
    if (!(m & W) || !(m & N)) m &= ~NW;
    return m;
}

/* =======================================================================
 * THE FIX-ME TABLE
 *
 * Each autotile element on the sheet is 12x4 blocks of 16x16 px.
 * LAYOUT[row][col] says which neighbor configuration lives at that block.
 *
 * If a ground tile looks wrong in game:
 *   1. find its case name below (EdgeN = open to the north, CornerNE = open
 *      to north+east, InSW = inner corner at south-west, EdgeN_InSW = north
 *      edge plus an inner corner at south-west, ...),
 *   2. move that name to the (col,row) of the right image block.
 *
 * Cols 0-3 are the classic 16 cases (verified against the sheet).
 * Cols 4-11 (the inner-corner combos) are educated guesses -- fix as needed.
 * (10,1) is empty on the sheet (NoCase). (9,2) is a spare plain full tile.
 * ======================================================================= */
constexpr Case LAYOUT[ROWS][COLS] = {
    /* row 0 */ {VCapT,   CornerNW_InSE, EdgeN_InSW_SE,   CornerNE_InSW, /**/ InNE_SE_SW,   EdgeN_InSW,  EdgeN_InSE,    InNW_SE_SW,    CornerNW,   InNW_NE,    EdgeN,      CornerNE},
    /* row 1 */ {VStripM, EdgeW_InNE_SE,         InAll,   EdgeE_InNW,    /**/ EdgeW_InNE,   InNW,        InNE,          EdgeE_InSW,    EdgeW,      InNW_SE,    NoCase,     InNE_SE},
    /* row 2 */ {VCapB,   CornerSW_InNE, EdgeS_InNW_NE,   CornerSE_InNW, /**/ EdgeW_InSE,   InSW,        InSE,          EdgeE_InNW_SW, InSW_NW,    Center,     InNE_SW,    EdgeE},
    /* row 3 */ {Iso,     HStripL,       HStripM,         HStripR,       /**/ InNW_NE_SE,   EdgeS_InNW,  EdgeS_InNE,    InNW_NE_SW,    CornerSW,   EdgeS,      InSE_SW,    CornerSE},
};

struct Tables {
    int8_t slotOf[CaseCount];  // Case -> block index (row*COLS+col) inside an element
    int8_t caseOf[256];        // canonical mask -> Case
    Tables() {
        for (auto& s : slotOf) s = -1;
        for (auto& c : caseOf) c = -1;
        for (int row = 0; row < ROWS; ++row)
            for (int col = 0; col < COLS; ++col) {
                const Case c = LAYOUT[row][col];
                if (c == NoCase) continue;
                if (slotOf[c] < 0) slotOf[c] = static_cast<int8_t>(row * COLS + col);  // first wins
            }
        for (int c = 0; c < CaseCount; ++c)
            if (slotOf[c] >= 0) caseOf[canon(CASE_MASK[c])] = static_cast<int8_t>(c);
    }
};

const Tables& tables() {
    static const Tables t;
    return t;
}

}  // namespace

int autotile::caseForMask(Uint8 mask) {
    const Tables& t = tables();
    Uint8 m = canon(mask);
    int c = t.caseOf[m];
    // Shape not on the sheet (with the default LAYOUT: InNE_SE_SW and InAll):
    // drop inner-corner bits one at a time until a known shape emerges.
    while (c < 0) {
        if (m & SE) m &= ~SE;
        else if (m & SW) m &= ~SW;
        else if (m & NE) m &= ~NE;
        else if (m & NW) m &= ~NW;
        else return Center;  // no corners left and still unknown: safety net
        c = t.caseOf[m];
    }
    return c;
}

SDL_FRect autotile::srcRect(int elemCol, int elemRow, int caseIdx) {
    const int slot = tables().slotOf[caseIdx];
    const int sc = slot % COLS, sr = slot / COLS;
    return SDL_FRect{static_cast<float>((elemCol * COLS + sc) * TILE_SIZE),
                     static_cast<float>((elemRow * ROWS + sr) * TILE_SIZE),
                     static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE)};
}
