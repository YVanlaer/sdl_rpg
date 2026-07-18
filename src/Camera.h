#pragma once

// World-space view rectangle. The game renders at a fixed logical resolution
// and the camera simply says which part of the world is visible.
struct Camera {
    float x = 0.0f;
    float y = 0.0f;
    float w = 480.0f;
    float h = 270.0f;
};
