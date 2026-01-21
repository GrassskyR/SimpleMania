#pragma once

#include <SDL.h>

#include <string>
#include <vector>

#include "Game.h"

struct RenderConfig {
    // 窗口与游玩区域布局
    int windowWidth = 900;
    int windowHeight = 600;
    int playWidth = 900;
    int playHeight = 600;
    int offsetX = 0;
    int judgeLineY = 480;
    int noteHeight = 12;
    int lanePadding = 2;
};

// 渲染游玩界面
void RenderFrame(SDL_Renderer* renderer, const Game& game, int nowMs, float scrollSpeed,
                 const RenderConfig& config, bool showStartOverlay);

// 渲染谱面选择菜单
void RenderMenu(SDL_Renderer* renderer, const RenderConfig& config,
                const std::vector<std::string>& items, int selectedIndex);

// 渲染暂停菜单
void RenderPauseMenu(SDL_Renderer* renderer, const RenderConfig& config, int selectedIndex);

// 渲染倒计时数字
void RenderCountdown(SDL_Renderer* renderer, const RenderConfig& config, int number);
