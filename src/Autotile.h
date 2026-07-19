#pragma once

#include <SDL3/SDL.h>

// Blob-style autotile lookup for sheets laid out like "Tileset Grass Spring.png".
//
// Such a sheet is a grid of "elements": each element is 12x4 blocks of 16x16 px
// and holds the tile shapes needed to draw one terrain with proper transitions
// (center, edges, outer corners, inner corners, strips, isolated tile).
// Which block holds which shape is described by the LAYOUT table in
// Autotile.cpp -- THAT is the table to edit when a tile looks wrong.
//
// Usage: build the 8-neighbor mask of a tile (bit set = neighbor of the SAME
// terrain class), then caseForMask() tells which shape to draw and srcRect()
// where that shape lives on the sheet.
namespace autotile {

// Neighbor mask bits.
constexpr Uint8 N = 1, E = 2, S = 4, W = 8;
constexpr Uint8 NE = 16, SE = 32, SW = 64, NW = 128;

constexpr int TILE_SIZE = 16;
constexpr int COLS = 12;  // blocks per element (width)
constexpr int ROWS = 4;   // blocks per element (height)

// The 47 canonical blob shapes. "In" = inner corner (notch) at the named
// corner(s), e.g. EdgeN_InSW = north edge with an inner corner at south-west,
// CornerNW_InSE = outer corner north-west plus inner corner at south-east.
// NoCase = slot left empty on the sheet.
enum Case : int {
    Center = 0,
    EdgeN, EdgeE, EdgeS, EdgeW,            // 1 open side
    CornerNW, CornerNE, CornerSE, CornerSW, // 2 adjacent open sides
    HStripM, VStripM,                       // 2 opposite open sides
    HStripL, HStripR, VCapT, VCapB,         // 3 open sides
    Iso,                                    // 4 open sides
    InNW, InNE, InSE, InSW,                 // center + 1 inner corner
    InNW_NE, InNE_SE, InSE_SW, InSW_NW,     // center + 2 adjacent inner corners
    InNW_SE, InNE_SW,                       // center + 2 diagonal inner corners
    InNW_NE_SE, InNW_NE_SW, InNW_SE_SW, InNE_SE_SW, // center + 3
    InAll,                                  // center + 4
    EdgeN_InSW, EdgeN_InSE, EdgeN_InSW_SE,  // edge + inner corner(s)
    EdgeE_InNW, EdgeE_InSW, EdgeE_InNW_SW,
    EdgeS_InNW, EdgeS_InNE, EdgeS_InNW_NE,
    EdgeW_InNE, EdgeW_InSE, EdgeW_InNE_SE,
    CornerNW_InSE, CornerNE_InSW, CornerSE_InNW, CornerSW_InNE,
    CaseCount,
    NoCase = -1,
};

// mask (any combination of the bits above) -> Case to draw. Corner bits are
// dropped unless both adjacent cardinal bits are set; shapes missing from the
// LAYOUT table fall back to a simpler shape, so any mask yields a valid Case.
int caseForMask(Uint8 mask);

// caseIdx + element position (in elements, 0-based) -> source rect in pixels.
SDL_FRect srcRect(int elemCol, int elemRow, int caseIdx);

}  // namespace autotile
