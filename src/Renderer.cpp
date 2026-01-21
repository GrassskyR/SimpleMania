#include "Renderer.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace {
SDL_Color LaneColor(int lane, int keyCount) {
    // 轨道底色（目前全黑）
    (void)lane;
    (void)keyCount;
    return SDL_Color{0, 0, 0, 255};
}

struct Glyph {
    char ch;
    unsigned char rows[7];
};

const Glyph kFont[] = {
    {'A', {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'B', {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E}},
    {'C', {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}},
    {'D', {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E}},
    {'E', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}},
    {'F', {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10}},
    {'G', {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0E}},
    {'H', {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}},
    {'I', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1F}},
    {'J', {0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C}},
    {'K', {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}},
    {'L', {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F}},
    {'M', {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}},
    {'N', {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11}},
    {'O', {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'P', {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10}},
    {'Q', {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}},
    {'R', {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11}},
    {'S', {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}},
    {'T', {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04}},
    {'U', {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}},
    {'V', {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04}},
    {'W', {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}},
    {'X', {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11}},
    {'Y', {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}},
    {'Z', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F}},
    {'0', {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E}},
    {'1', {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}},
    {'2', {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F}},
    {'3', {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E}},
    {'4', {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02}},
    {'5', {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E}},
    {'6', {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E}},
    {'7', {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08}},
    {'8', {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E}},
    {'9', {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E}},
    {':', {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00}},
    {'+', {0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00}},
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
    {'/', {0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'%', {0x19, 0x1A, 0x04, 0x08, 0x16, 0x07, 0x00}},
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
};

const Glyph* FindGlyph(char c) {
    // 查找字形
    if (c >= 'a' && c <= 'z') {
        c = static_cast<char>(c - 'a' + 'A');
    }
    for (const auto& glyph : kFont) {
        if (glyph.ch == c) {
            return &glyph;
        }
    }
    return nullptr;
}

void DrawText(SDL_Renderer* renderer, int x, int y, int scale, SDL_Color color, const std::string& text) {
    // 使用简易5x7像素字体绘制文本
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int cursorX = x;
    for (char c : text) {
        const Glyph* glyph = FindGlyph(c);
        if (!glyph) {
            cursorX += 6 * scale;
            continue;
        }
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph->rows[row] & (1 << (4 - col))) {
                    SDL_Rect pixel{cursorX + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(renderer, &pixel);
                }
            }
        }
        cursorX += 6 * scale;
    }
}

std::string JudgeToString(JudgeGrade grade) {
    // 判定字符串
    switch (grade) {
        case JudgeGrade::Perfect:
            return "PERFECT";
        case JudgeGrade::Good:
            return "GOOD";
        case JudgeGrade::Miss:
            return "MISS";
        default:
            return "";
    }
}

SDL_Color JudgeColor(JudgeGrade grade) {
    // 判定颜色
    switch (grade) {
        case JudgeGrade::Perfect:
            return SDL_Color{245, 200, 70, 255};
        case JudgeGrade::Good:
            return SDL_Color{80, 170, 255, 255};
        case JudgeGrade::Miss:
            return SDL_Color{235, 80, 80, 255};
        default:
            return SDL_Color{240, 240, 240, 255};
    }
}
}

void RenderFrame(SDL_Renderer* renderer, const Game& game, int nowMs, float scrollSpeed,
                 const RenderConfig& config, bool showStartOverlay) {
    // 游戏画面渲染
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    int keyCount = std::max(1, game.GetKeyCount());
    float laneWidth = static_cast<float>(config.playWidth) / static_cast<float>(keyCount);

    for (int lane = 0; lane < keyCount; ++lane) {
        // 绘制轨道
        SDL_Color color = LaneColor(lane, keyCount);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
        SDL_Rect laneRect{
            static_cast<int>(config.offsetX + lane * laneWidth + config.lanePadding),
            0,
            static_cast<int>(laneWidth - config.lanePadding * 2),
            config.playHeight
        };
        SDL_RenderFillRect(renderer, &laneRect);
    }

    SDL_SetRenderDrawColor(renderer, 220, 220, 230, 255);
    SDL_Rect judgeLine{config.offsetX, config.judgeLineY - 2, config.playWidth, 4};
    SDL_RenderFillRect(renderer, &judgeLine);

    const auto& notes = game.GetNotes();
    for (const auto& note : notes) {
        // 绘制未判定音符
        if (note.judged) {
            continue;
        }
        float timeDiff = static_cast<float>(note.timeMs - nowMs);
        float y = static_cast<float>(config.judgeLineY) - timeDiff * scrollSpeed;
        if (y < -config.noteHeight || y > config.playHeight + config.noteHeight) {
            continue;
        }
        SDL_Rect noteRect{
            static_cast<int>(config.offsetX + note.lane * laneWidth + config.lanePadding + 4),
            static_cast<int>(y - config.noteHeight),
            static_cast<int>(laneWidth - config.lanePadding * 2 - 8),
            config.noteHeight
        };
        SDL_SetRenderDrawColor(renderer, 245, 180, 70, 255);
        SDL_RenderFillRect(renderer, &noteRect);
    }

    const GameStats& stats = game.GetStats();
    // HUD: 分数、速度、ACC、连击与判定

    SDL_Color textColor{240, 240, 240, 255};
    int totalScore = game.GetTotalScore();
    double acc = game.GetAccuracy();
    char scoreText[64];
    char accText[64];
    char speedText[64];
    std::snprintf(scoreText, sizeof(scoreText), "SCORE %d", totalScore);
    std::snprintf(accText, sizeof(accText), "ACC %05.2f%%", acc);
    std::snprintf(speedText, sizeof(speedText), "SPEED %.2f", scrollSpeed);

    DrawText(renderer, config.offsetX + 16, 16, 2, textColor, scoreText);
    DrawText(renderer, config.offsetX + 16, 40, 2, textColor, speedText);
    int accWidth = static_cast<int>(std::string(accText).size()) * 12;
    DrawText(renderer, config.offsetX + config.playWidth - accWidth - 16, 16, 2, textColor, accText);

    char comboText[64];
    std::snprintf(comboText, sizeof(comboText), "COMBO %d", stats.combo);
    int comboWidth = static_cast<int>(std::string(comboText).size()) * 12;
    DrawText(renderer, config.offsetX + config.playWidth / 2 - comboWidth / 2, 56, 2, textColor, comboText);

    if (stats.lastJudge != JudgeGrade::None && (nowMs - stats.lastJudgeTimeMs) < 1000) {
        std::string judgeText = JudgeToString(stats.lastJudge);
        int judgeScale = 3;
        int judgeWidth = static_cast<int>(judgeText.size()) * 6 * judgeScale;
        int judgeX = config.offsetX + config.playWidth / 2 - judgeWidth / 2;
        int judgeY = config.playHeight / 2 + 40;
        DrawText(renderer, judgeX, judgeY, judgeScale, JudgeColor(stats.lastJudge), judgeText);
    }

    if (showStartOverlay) {
        // 未开始时的播放遮罩与按钮
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 10, 14, 180);
        SDL_Rect overlay{0, 0, config.windowWidth, config.windowHeight};
        SDL_RenderFillRect(renderer, &overlay);

        SDL_Rect button{
            config.windowWidth / 2 - 90,
            config.windowHeight / 2 - 30,
            180,
            60
        };
        SDL_SetRenderDrawColor(renderer, 240, 200, 80, 255);
        SDL_RenderFillRect(renderer, &button);
        SDL_SetRenderDrawColor(renderer, 40, 30, 20, 255);
        SDL_RenderDrawRect(renderer, &button);

        SDL_Color playText{30, 20, 10, 255};
        DrawText(renderer, config.windowWidth / 2 - 30, config.windowHeight / 2 - 10, 3, playText, "PLAY");
    }

}

void RenderMenu(SDL_Renderer* renderer, const RenderConfig& config,
                const std::vector<std::string>& items, int selectedIndex) {
    // 菜单渲染
    SDL_SetRenderDrawColor(renderer, 14, 14, 20, 255);
    SDL_RenderClear(renderer);

    SDL_Color titleColor{235, 225, 210, 255};
    DrawText(renderer, 24, 24, 3, titleColor, "SELECT BEATMAP");

    SDL_Color hintColor{180, 180, 180, 255};
    DrawText(renderer, 24, 60, 2, hintColor, "UP/DOWN: SELECT  ENTER: PLAY");
    DrawText(renderer, 24, 82, 2, hintColor, "CTRL +/-: SPEED  F5: RESOLUTION  ESC: QUIT");
    DrawText(renderer, 24, 104, 2, hintColor, "KEYS 4K DFJK  5K DF SPACE JK  6K SDF JKL  7K SDF SPACE JKL");

    if (items.empty()) {
        SDL_Color warnColor{220, 120, 120, 255};
        DrawText(renderer, 24, 80, 2, warnColor, "NO OSU FILES FOUND");
        return;
    }

    int startY = 140;
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        SDL_Color color = (i == selectedIndex) ? SDL_Color{240, 200, 80, 255}
                                                : SDL_Color{220, 220, 220, 255};
        DrawText(renderer, 40, startY + i * 26, 2, color, items[i]);
    }

}

void RenderPauseMenu(SDL_Renderer* renderer, const RenderConfig& config, int selectedIndex) {
    // 暂停菜单渲染
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 192);
    SDL_Rect overlay{0, 0, config.windowWidth, config.windowHeight};
    SDL_RenderFillRect(renderer, &overlay);

    SDL_Color titleColor{240, 240, 240, 255};
    int centerX = config.windowWidth / 2;
    int titleScale = 3;
    std::string titleText = "PAUSED";
    int titleWidth = static_cast<int>(titleText.size()) * 6 * titleScale;
    DrawText(renderer, centerX - titleWidth / 2, config.windowHeight / 2 - 70, titleScale, titleColor, titleText);

    SDL_Color normal{220, 220, 220, 255};
    SDL_Color active{245, 200, 80, 255};
    SDL_Color resumeColor = selectedIndex == 0 ? active : normal;
    SDL_Color menuColor = selectedIndex == 1 ? active : normal;
    int optionScale = 2;
    std::string resumeText = "RESUME";
    std::string menuText = "BACK TO MENU";
    int resumeWidth = static_cast<int>(resumeText.size()) * 6 * optionScale;
    int menuWidth = static_cast<int>(menuText.size()) * 6 * optionScale;
    DrawText(renderer, centerX - resumeWidth / 2, config.windowHeight / 2 - 10, optionScale, resumeColor, resumeText);
    DrawText(renderer, centerX - menuWidth / 2, config.windowHeight / 2 + 20, optionScale, menuColor, menuText);

}

void RenderCountdown(SDL_Renderer* renderer, const RenderConfig& config, int number) {
    // 倒计时数字
    SDL_Color color{255, 255, 255, 255};
    int scale = 8;
    std::string text = std::to_string(number);
    int textWidth = static_cast<int>(text.size()) * 6 * scale;
    int x = config.windowWidth / 2 - textWidth / 2;
    int y = config.windowHeight / 2 - (7 * scale) / 2;
    DrawText(renderer, x, y, scale, color, text);
}
