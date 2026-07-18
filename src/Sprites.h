#pragma once

#include <SDL3/SDL.h>

#include "Camera.h"

enum class Direction { Down, Up, Left, Right };

// Grid-based character sprite definition: uniform cells of fw x fh on one
// sheet. Rows are [down, up, left] starting at rowBase; right faces flip the
// left row. When fixedRow is set, rowBase is used as the absolute row and no
// flipping is applied (used for the merchant).
struct CharDef {
    SDL_Texture* img = nullptr;
    int fw = 32;
    int fh = 32;
    int frames = 4;
    int fps = 6;
    int rowBase = 0;
    bool fixedRow = false;
};

// Screen-space sprite draw. (x, y) is the top-left corner; positions are
// rounded to whole pixels to keep the pixel art crisp.
void drawSprite(SDL_Renderer* r, SDL_Texture* tex, const SDL_FRect* src,
                float x, float y, float w, float h, bool flip = false);

// World-space variants: the camera offset is subtracted before drawing.
void drawSpriteW(SDL_Renderer* r, const Camera& cam, SDL_Texture* tex, const SDL_FRect* src,
                 float x, float y, float w, float h, bool flip = false);

// Draws one animation frame anchored bottom-center at world position (x, y).
void drawCharFrame(SDL_Renderer* r, const Camera& cam, const CharDef& def,
                   Direction dir, int frame, float x, float y, float scale = 1.0f);

// Soft ellipse shadow under a character, in world space.
void drawShadow(SDL_Renderer* r, const Camera& cam, float x, float y, float w);

// Filled ellipse (scanline fill), screen space. Used for shadows.
void fillEllipse(SDL_Renderer* r, float cx, float cy, float rx, float ry);
