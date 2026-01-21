#include "Renderer.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace {
SDL_Color LaneColor(int lane, int keyCount) {
    if (keyCount <= 1) {
        return SDL_Color{60, 60, 70, 255};
    }
    float t = static_cast<float>(lane) / static_cast<float>(keyCount - 1);
    Uint8 r = static_cast<Uint8>(40 + 30 * t);
    Uint8 g = static_cast<Uint8>(50 + 60 * (1.0f - t));
    Uint8 b = static_cast<Uint8>(70 + 40 * t);
    return SDL_Color{r, g, b, 255};
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
    {'.', {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C}},
    {'-', {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}},
    {'%', {0x19, 0x1A, 0x04, 0x08, 0x16, 0x07, 0x00}},
    {' ', {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}
};

const Glyph* FindGlyph(char c) {
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
    SDL_SetRenderDrawColor(renderer, 18, 18, 24, 255);
    SDL_RenderClear(renderer);

    int keyCount = std::max(1, game.GetKeyCount());
    float laneWidth = static_cast<float>(config.width) / static_cast<float>(keyCount);

    for (int lane = 0; lane < keyCount; ++lane) {
        SDL_Color color = LaneColor(lane, keyCount);
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
        SDL_Rect laneRect{
            static_cast<int>(lane * laneWidth + config.lanePadding),
            0,
            static_cast<int>(laneWidth - config.lanePadding * 2),
            config.height
        };
        SDL_RenderFillRect(renderer, &laneRect);
    }

    SDL_SetRenderDrawColor(renderer, 220, 220, 230, 255);
    SDL_Rect judgeLine{0, config.judgeLineY - 2, config.width, 4};
    SDL_RenderFillRect(renderer, &judgeLine);

    const auto& notes = game.GetNotes();
    for (const auto& note : notes) {
        if (note.judged) {
            continue;
        }
        float timeDiff = static_cast<float>(note.timeMs - nowMs);
        float y = static_cast<float>(config.judgeLineY) - timeDiff * scrollSpeed;
        if (y < -config.noteHeight || y > config.height + config.noteHeight) {
            continue;
        }
        SDL_Rect noteRect{
            static_cast<int>(note.lane * laneWidth + config.lanePadding + 4),
            static_cast<int>(y - config.noteHeight),
            static_cast<int>(laneWidth - config.lanePadding * 2 - 8),
            config.noteHeight
        };
        SDL_SetRenderDrawColor(renderer, 245, 180, 70, 255);
        SDL_RenderFillRect(renderer, &noteRect);
    }

    const GameStats& stats = game.GetStats();

    SDL_Color textColor{240, 240, 240, 255};
    int totalScore = game.GetTotalScore();
    double acc = game.GetAccuracy();
    char scoreText[64];
    char accText[64];
    std::snprintf(scoreText, sizeof(scoreText), "SCORE %d", totalScore);
    std::snprintf(accText, sizeof(accText), "ACC %05.2f%%", acc);

    DrawText(renderer, 16, 16, 2, textColor, scoreText);
    int accWidth = static_cast<int>(std::string(accText).size()) * 12;
    DrawText(renderer, config.width - accWidth - 16, 16, 2, textColor, accText);

    char comboText[64];
    std::snprintf(comboText, sizeof(comboText), "COMBO %d", stats.combo);
    int comboWidth = static_cast<int>(std::string(comboText).size()) * 12;
    DrawText(renderer, config.width / 2 - comboWidth / 2, 56, 2, textColor, comboText);

    if (stats.lastJudge != JudgeGrade::None && (nowMs - stats.lastJudgeTimeMs) < 1000) {
        std::string judgeText = JudgeToString(stats.lastJudge);
        int judgeScale = 3;
        int judgeWidth = static_cast<int>(judgeText.size()) * 6 * judgeScale;
        int judgeX = config.width / 2 - judgeWidth / 2;
        int judgeY = config.height / 2 + 40;
        DrawText(renderer, judgeX, judgeY, judgeScale, JudgeColor(stats.lastJudge), judgeText);
    }

    if (showStartOverlay) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 10, 10, 14, 180);
        SDL_Rect overlay{0, 0, config.width, config.height};
        SDL_RenderFillRect(renderer, &overlay);

        SDL_Rect button{
            config.width / 2 - 90,
            config.height / 2 - 30,
            180,
            60
        };
        SDL_SetRenderDrawColor(renderer, 240, 200, 80, 255);
        SDL_RenderFillRect(renderer, &button);
        SDL_SetRenderDrawColor(renderer, 40, 30, 20, 255);
        SDL_RenderDrawRect(renderer, &button);

        SDL_Color playText{30, 20, 10, 255};
        DrawText(renderer, config.width / 2 - 30, config.height / 2 - 10, 3, playText, "PLAY");
    }

    SDL_RenderPresent(renderer);
}
