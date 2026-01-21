#include <SDL.h>
#ifdef USE_SDL_MIXER
#include <SDL_mixer.h>
#endif

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "Game.h"
#include "OsuParser.h"
#include "Renderer.h"

namespace {
std::string GetDirectory(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return "";
    }
    return path.substr(0, pos + 1);
}

std::vector<SDL_Scancode> BuildKeyMap(int keyCount) {
    if (keyCount == 4) {
        return {SDL_SCANCODE_D, SDL_SCANCODE_F, SDL_SCANCODE_J, SDL_SCANCODE_K};
    }
    if (keyCount == 5) {
        return {SDL_SCANCODE_D, SDL_SCANCODE_F, SDL_SCANCODE_SPACE, SDL_SCANCODE_J, SDL_SCANCODE_K};
    }
    if (keyCount == 6) {
        return {SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F, SDL_SCANCODE_J, SDL_SCANCODE_K, SDL_SCANCODE_L};
    }
    if (keyCount == 7) {
        return {SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F, SDL_SCANCODE_SPACE,
                SDL_SCANCODE_J, SDL_SCANCODE_K, SDL_SCANCODE_L};
    }
    std::string letters = "ASDFGHJKLQWERTYUIOPZXCVBNM";
    std::vector<SDL_Scancode> map;
    map.reserve(keyCount);
    for (int i = 0; i < keyCount; ++i) {
        if (i < static_cast<int>(letters.size())) {
            char c = letters[i];
            map.push_back(static_cast<SDL_Scancode>(SDL_SCANCODE_A + (c - 'A')));
        } else {
            map.push_back(SDL_SCANCODE_UNKNOWN);
        }
    }
    return map;
}

SDL_Rect GetPlayButtonRect(const RenderConfig& config) {
    return SDL_Rect{config.width / 2 - 90, config.height / 2 - 30, 180, 60};
}
}

int main(int argc, char* argv[]) {
    std::string osuPath = argc > 1 ? argv[1] : "assets/demo.osu";

    Chart chart;
    std::string error;
    if (!ParseOsuFile(osuPath, chart, error)) {
        std::printf("Failed to load chart: %s\n", error.c_str());
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        std::printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    RenderConfig renderConfig;
    SDL_Window* window = SDL_CreateWindow(
        "SimpleMania", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        renderConfig.width, renderConfig.height, SDL_WINDOW_SHOWN);
    if (!window) {
        std::printf("Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::printf("Renderer creation failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::string audioPath;
    if (!chart.audioFilename.empty()) {
        audioPath = GetDirectory(osuPath) + chart.audioFilename;
    }

#ifdef USE_SDL_MIXER
    Mix_Music* music = nullptr;
    if (!audioPath.empty()) {
        Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG);
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == 0) {
            music = Mix_LoadMUS(audioPath.c_str());
            if (!music) {
                std::printf("Failed to load music: %s\n", Mix_GetError());
            }
        } else {
            std::printf("Mix_OpenAudio failed: %s\n", Mix_GetError());
        }
    }
#else
    SDL_AudioDeviceID audioDevice = 0;
    SDL_AudioSpec wavSpec{};
    Uint8* wavBuffer = nullptr;
    Uint32 wavLength = 0;
    if (!audioPath.empty()) {
        if (SDL_LoadWAV(audioPath.c_str(), &wavSpec, &wavBuffer, &wavLength)) {
            audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wavSpec, nullptr, 0);
        } else {
            std::printf("Audio load failed (WAV only in this build): %s\n", SDL_GetError());
        }
    }
#endif

    Game game;
    game.LoadChart(chart);

    std::vector<SDL_Scancode> keyMap = BuildKeyMap(game.GetKeyCount());
    float scrollSpeed = 0.5f;

    Uint32 startTime = 0;
    bool started = false;
    SDL_Rect playButton = GetPlayButtonRect(renderConfig);
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                if (!started) {
                    SDL_Point point{event.button.x, event.button.y};
                    if (SDL_PointInRect(&point, &playButton)) {
                        started = true;
                        startTime = SDL_GetTicks();
#ifdef USE_SDL_MIXER
                        if (music) {
                            Mix_PlayMusic(music, 0);
                        }
#else
                        if (audioDevice != 0 && wavBuffer) {
                            SDL_ClearQueuedAudio(audioDevice);
                            SDL_QueueAudio(audioDevice, wavBuffer, wavLength);
                            SDL_PauseAudioDevice(audioDevice, 0);
                        }
#endif
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Scancode code = event.key.keysym.scancode;
                if (code == SDL_SCANCODE_ESCAPE) {
                    running = false;
                } else if (!started && (code == SDL_SCANCODE_SPACE || code == SDL_SCANCODE_RETURN)) {
                    started = true;
                    startTime = SDL_GetTicks();
#ifdef USE_SDL_MIXER
                    if (music) {
                        Mix_PlayMusic(music, 0);
                    }
#else
                    if (audioDevice != 0 && wavBuffer) {
                        SDL_ClearQueuedAudio(audioDevice);
                        SDL_QueueAudio(audioDevice, wavBuffer, wavLength);
                        SDL_PauseAudioDevice(audioDevice, 0);
                    }
#endif
                } else if (code == SDL_SCANCODE_UP) {
                    scrollSpeed = std::min(2.5f, scrollSpeed + 0.05f);
                } else if (code == SDL_SCANCODE_DOWN) {
                    scrollSpeed = std::max(0.1f, scrollSpeed - 0.05f);
                } else if (started) {
                    for (int lane = 0; lane < static_cast<int>(keyMap.size()); ++lane) {
                        if (keyMap[lane] == code) {
                            int nowMs = static_cast<int>(SDL_GetTicks() - startTime);
                            game.HandleInput(lane, nowMs);
                            break;
                        }
                    }
                }
            }
        }

        int nowMs = 0;
        if (started) {
            nowMs = static_cast<int>(SDL_GetTicks() - startTime);
            game.Update(nowMs);
        }
        RenderFrame(renderer, game, nowMs, scrollSpeed, renderConfig, !started);

        const GameStats& stats = game.GetStats();
        char title[256];
        if (!started) {
            std::snprintf(title, sizeof(title), "SimpleMania | Click Play or Press Space");
        } else {
            std::snprintf(title, sizeof(title), "SimpleMania | Score %d | Combo %d | Speed %.2f",
                          game.GetTotalScore(), stats.combo, scrollSpeed);
        }
        SDL_SetWindowTitle(window, title);

        SDL_Delay(16);
    }

#ifdef USE_SDL_MIXER
    if (music) {
        Mix_FreeMusic(music);
    }
    Mix_CloseAudio();
    Mix_Quit();
#else
    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
    }
    if (wavBuffer) {
        SDL_FreeWAV(wavBuffer);
    }
#endif
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
