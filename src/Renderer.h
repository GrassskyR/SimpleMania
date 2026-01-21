#pragma once

#include <SDL.h>

#include "Game.h"

struct RenderConfig {
    int width = 900;
    int height = 600;
    int judgeLineY = 480;
    int noteHeight = 12;
    int lanePadding = 2;
};

void RenderFrame(SDL_Renderer* renderer, const Game& game, int nowMs, float scrollSpeed,
                 const RenderConfig& config, bool showStartOverlay);
