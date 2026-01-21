#include <SDL.h>
#ifdef USE_SDL_MIXER
#include <SDL_mixer.h>
#endif

#include <algorithm>
#include <cstdio>
#include <filesystem>
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
    return SDL_Rect{config.windowWidth / 2 - 90, config.windowHeight / 2 - 30, 180, 60};
}

struct ChartEntry {
    std::string label;
    std::string path;
};

struct ResolutionOption {
    int width = 0;
    int height = 0;
};

std::vector<ChartEntry> ScanCharts(const std::string& rootPath) {
    std::vector<ChartEntry> entries;
    std::error_code error;
    if (!std::filesystem::exists(rootPath, error)) {
        std::filesystem::create_directories(rootPath, error);
        return entries;
    }

    for (const auto& dirEntry : std::filesystem::directory_iterator(rootPath)) {
        if (!dirEntry.is_directory()) {
            continue;
        }
        std::string folder = dirEntry.path().filename().string();
        for (const auto& fileEntry : std::filesystem::directory_iterator(dirEntry.path())) {
            if (!fileEntry.is_regular_file()) {
                continue;
            }
            if (fileEntry.path().extension() == ".osu") {
                ChartEntry entry;
                entry.path = fileEntry.path().string();
                Chart metadata;
                std::string errorText;
                if (ParseOsuFile(entry.path, metadata, errorText)) {
                    if (!metadata.title.empty() && !metadata.version.empty()) {
                        entry.label = metadata.title + " - " + metadata.version;
                    } else if (!metadata.title.empty()) {
                        entry.label = metadata.title;
                    }
                }
                if (entry.label.empty()) {
                    entry.label = fileEntry.path().stem().string();
                }
                entries.push_back(entry);
            }
        }
    }

    std::sort(entries.begin(), entries.end(), [](const ChartEntry& a, const ChartEntry& b) {
        return a.label < b.label;
    });
    return entries;
}
}

int main(int argc, char* argv[]) {
    // 主入口：初始化SDL、加载菜单与游戏循环
    std::string osuPath = argc > 1 ? argv[1] : "";

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) != 0) {
        std::printf("SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    RenderConfig renderConfig;
    std::vector<ResolutionOption> resolutions = {
        {900, 600},
        {1280, 720},
        {1600, 900},
        {1920, 1080}
    };
    int resolutionIndex = 0;
    renderConfig.windowWidth = resolutions[resolutionIndex].width;
    renderConfig.windowHeight = resolutions[resolutionIndex].height;
    renderConfig.playWidth = 900;
    renderConfig.playHeight = renderConfig.windowHeight;
    renderConfig.offsetX = (renderConfig.windowWidth - renderConfig.playWidth) / 2;
    renderConfig.judgeLineY = renderConfig.playHeight - 80;

    SDL_Window* window = SDL_CreateWindow(
        "SimpleMania", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        renderConfig.windowWidth, renderConfig.windowHeight, SDL_WINDOW_SHOWN);
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

#ifdef USE_SDL_MIXER
    Mix_Music* music = nullptr;
    Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG);
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096) != 0) {
        std::printf("Mix_OpenAudio failed: %s\n", Mix_GetError());
    }
#else
    SDL_AudioDeviceID audioDevice = 0;
    SDL_AudioSpec wavSpec{};
    Uint8* wavBuffer = nullptr;
    Uint32 wavLength = 0;
#endif

    enum class AppState {
        Menu,
        Ready,
        Countdown,
        Playing,
        Paused
    };

    // 扫描assets子目录下的osu谱面
    std::vector<ChartEntry> chartEntries = ScanCharts("assets");
    int selectedIndex = 0;
    Chart chart;
    Game game;
    std::vector<SDL_Scancode> keyMap;
    float scrollSpeed = 1.0f;
    Uint32 startTicks = 0;
    Uint32 pauseStartTicks = 0;
    Uint32 countdownStartTicks = 0;
    int timeOffsetMs = 0;
    int pausedGameTimeMs = 0;
    bool countdownFromPause = false;
    SDL_Rect playButton = GetPlayButtonRect(renderConfig);
    AppState state = AppState::Menu;
    int pauseMenuIndex = 0;
    const int countdownDurationMs = 3000;

    // 释放当前音频资源
    auto unloadAudio = [&]() {
#ifdef USE_SDL_MIXER
        if (music) {
            Mix_HaltMusic();
            Mix_FreeMusic(music);
            music = nullptr;
        }
#else
        if (audioDevice != 0) {
            SDL_CloseAudioDevice(audioDevice);
            audioDevice = 0;
        }
        if (wavBuffer) {
            SDL_FreeWAV(wavBuffer);
            wavBuffer = nullptr;
            wavLength = 0;
        }
#endif
    };

    // 读取谱面与音频资源
    auto loadChart = [&](const std::string& path) -> bool {
        Chart nextChart;
        std::string error;
        if (!ParseOsuFile(path, nextChart, error)) {
            std::printf("Failed to load chart: %s\n", error.c_str());
            return false;
        }

        unloadAudio();
        std::string audioPath;
        if (!nextChart.audioFilename.empty()) {
            audioPath = GetDirectory(path) + nextChart.audioFilename;
        }

#ifdef USE_SDL_MIXER
        if (!audioPath.empty()) {
            music = Mix_LoadMUS(audioPath.c_str());
            if (!music) {
                std::printf("Failed to load music: %s\n", Mix_GetError());
            }
        }
#else
        if (!audioPath.empty()) {
            if (SDL_LoadWAV(audioPath.c_str(), &wavSpec, &wavBuffer, &wavLength)) {
                audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wavSpec, nullptr, 0);
            } else {
                std::printf("Audio load failed (WAV only in this build): %s\n", SDL_GetError());
            }
        }
#endif

        chart = nextChart;
        game.LoadChart(chart);
        keyMap = BuildKeyMap(game.GetKeyCount());
        return true;
    };

    // 返回菜单并重置状态
    auto returnToMenu = [&]() {
        unloadAudio();
        startTicks = 0;
        pauseStartTicks = 0;
        countdownStartTicks = 0;
        timeOffsetMs = 0;
        pausedGameTimeMs = 0;
        countdownFromPause = false;
        pauseMenuIndex = 0;
        state = AppState::Menu;
    };

    auto startCountdown = [&](bool fromPause) {
        countdownFromPause = fromPause;
        countdownStartTicks = SDL_GetTicks();
        if (!fromPause) {
            startTicks = countdownStartTicks;
            timeOffsetMs = 0;
            pausedGameTimeMs = 0;
        }
        state = AppState::Countdown;
    };

    auto startAudio = [&](bool restart) {
#ifdef USE_SDL_MIXER
        if (music) {
            if (restart) {
                Mix_HaltMusic();
                Mix_RewindMusic();
                Mix_PlayMusic(music, 0);
            } else if (Mix_PausedMusic()) {
                Mix_ResumeMusic();
            } else if (!Mix_PlayingMusic()) {
                Mix_PlayMusic(music, 0);
            }
        }
#else
        if (audioDevice != 0 && wavBuffer) {
            if (restart) {
                SDL_ClearQueuedAudio(audioDevice);
                SDL_QueueAudio(audioDevice, wavBuffer, wavLength);
            }
            SDL_PauseAudioDevice(audioDevice, 0);
        }
#endif
    };

    auto pauseAudio = [&]() {
#ifdef USE_SDL_MIXER
        if (music) {
            Mix_PauseMusic();
        }
#else
        if (audioDevice != 0) {
            SDL_PauseAudioDevice(audioDevice, 1);
        }
#endif
    };

    // 切换窗口分辨率（宽度固定900，增加高度）
    auto applyResolution = [&]() {
        renderConfig.windowWidth = resolutions[resolutionIndex].width;
        renderConfig.windowHeight = resolutions[resolutionIndex].height;
        renderConfig.playWidth = 900;
        renderConfig.playHeight = renderConfig.windowHeight;
        renderConfig.offsetX = (renderConfig.windowWidth - renderConfig.playWidth) / 2;
        renderConfig.judgeLineY = renderConfig.playHeight - 80;
        SDL_SetWindowSize(window, renderConfig.windowWidth, renderConfig.windowHeight);
        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        playButton = GetPlayButtonRect(renderConfig);
    };

    if (!osuPath.empty()) {
        if (loadChart(osuPath)) {
            state = AppState::Ready;
        }
    }
    const int targetFps = 165;
    const int targetFrameMs = 1000 / targetFps;
    std::vector<Uint8> prevKeys(SDL_NUM_SCANCODES, 0);
    bool running = true;
    // 主循环
    while (running) {
        Uint32 frameStart = SDL_GetTicks();
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                if (state == AppState::Ready) {
                    SDL_Point point{event.button.x, event.button.y};
                    if (SDL_PointInRect(&point, &playButton)) {
                        startCountdown(false);
                    }
                }
            } else if (event.type == SDL_KEYDOWN) {
                SDL_Scancode code = event.key.keysym.scancode;
                if (code == SDL_SCANCODE_ESCAPE) {
                    if (state == AppState::Menu) {
                        running = false;
                    } else if (state == AppState::Countdown) {
                        break;
                    } else if (state == AppState::Playing) {
                        pauseStartTicks = SDL_GetTicks();
                        pausedGameTimeMs = static_cast<int>(pauseStartTicks - startTicks - timeOffsetMs);
                        pauseMenuIndex = 0;
                        pauseAudio();
                        state = AppState::Paused;
                    } else if (state == AppState::Paused) {
                        startCountdown(true);
                    } else {
                        returnToMenu();
                    }
                } else if (code == SDL_SCANCODE_F5) {
                    resolutionIndex = (resolutionIndex + 1) % static_cast<int>(resolutions.size());
                    applyResolution();
                }
            }
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        Uint16 mods = SDL_GetModState();
        bool ctrlDown = (mods & KMOD_CTRL) != 0;
        if (state != AppState::Menu && ctrlDown) {
            if ((keys[SDL_SCANCODE_EQUALS] && !prevKeys[SDL_SCANCODE_EQUALS]) ||
                (keys[SDL_SCANCODE_KP_PLUS] && !prevKeys[SDL_SCANCODE_KP_PLUS])) {
                scrollSpeed = std::min(3.0f, scrollSpeed + 0.1f);
            } else if ((keys[SDL_SCANCODE_MINUS] && !prevKeys[SDL_SCANCODE_MINUS]) ||
                       (keys[SDL_SCANCODE_KP_MINUS] && !prevKeys[SDL_SCANCODE_KP_MINUS])) {
                scrollSpeed = std::max(0.1f, scrollSpeed - 0.1f);
            }
        }

        if (state == AppState::Menu && !chartEntries.empty()) {
            if (keys[SDL_SCANCODE_UP] && !prevKeys[SDL_SCANCODE_UP]) {
                selectedIndex = std::max(0, selectedIndex - 1);
            } else if (keys[SDL_SCANCODE_DOWN] && !prevKeys[SDL_SCANCODE_DOWN]) {
                selectedIndex = std::min(static_cast<int>(chartEntries.size()) - 1,
                                         selectedIndex + 1);
            } else if ((keys[SDL_SCANCODE_RETURN] && !prevKeys[SDL_SCANCODE_RETURN]) ||
                       (keys[SDL_SCANCODE_SPACE] && !prevKeys[SDL_SCANCODE_SPACE])) {
                if (loadChart(chartEntries[selectedIndex].path)) {
                    state = AppState::Ready;
                    pauseMenuIndex = 0;
                }
            }
        }

        if (state == AppState::Ready) {
            if ((keys[SDL_SCANCODE_RETURN] && !prevKeys[SDL_SCANCODE_RETURN]) ||
                (keys[SDL_SCANCODE_SPACE] && !prevKeys[SDL_SCANCODE_SPACE])) {
                startCountdown(false);
            }
        }

        if (state == AppState::Paused) {
            if (keys[SDL_SCANCODE_UP] && !prevKeys[SDL_SCANCODE_UP]) {
                pauseMenuIndex = std::max(0, pauseMenuIndex - 1);
            } else if (keys[SDL_SCANCODE_DOWN] && !prevKeys[SDL_SCANCODE_DOWN]) {
                pauseMenuIndex = std::min(1, pauseMenuIndex + 1);
            } else if ((keys[SDL_SCANCODE_RETURN] && !prevKeys[SDL_SCANCODE_RETURN]) ||
                       (keys[SDL_SCANCODE_SPACE] && !prevKeys[SDL_SCANCODE_SPACE])) {
                if (pauseMenuIndex == 0) {
                    startCountdown(true);
                } else {
                    returnToMenu();
                }
            }
        }

        if (state == AppState::Countdown) {
            Uint32 elapsed = SDL_GetTicks() - countdownStartTicks;
            if (elapsed >= static_cast<Uint32>(countdownDurationMs)) {
                if (countdownFromPause) {
                    timeOffsetMs += static_cast<int>(SDL_GetTicks() - pauseStartTicks);
                } else {
                    timeOffsetMs += countdownDurationMs;
                }
                startAudio(!countdownFromPause);
                countdownFromPause = false;
                state = AppState::Playing;
            }
        }

        if (state == AppState::Playing) {
            for (int lane = 0; lane < static_cast<int>(keyMap.size()); ++lane) {
                SDL_Scancode scancode = keyMap[lane];
                if (scancode != SDL_SCANCODE_UNKNOWN && keys[scancode] && !prevKeys[scancode]) {
                    int nowMs = static_cast<int>(SDL_GetTicks() - startTicks - timeOffsetMs);
                    game.HandleInput(lane, nowMs);
                }
            }
        }

        int nowMs = 0;
        if (state == AppState::Playing) {
            nowMs = static_cast<int>(SDL_GetTicks() - startTicks - timeOffsetMs);
            game.Update(nowMs);
            RenderFrame(renderer, game, nowMs, scrollSpeed, renderConfig, false);
        } else if (state == AppState::Ready) {
            RenderFrame(renderer, game, 0, scrollSpeed, renderConfig, true);
        } else if (state == AppState::Countdown) {
            int renderTime = countdownFromPause ? pausedGameTimeMs : 0;
            RenderFrame(renderer, game, renderTime, scrollSpeed, renderConfig, false);
            Uint32 elapsed = SDL_GetTicks() - countdownStartTicks;
            int remaining = countdownDurationMs - static_cast<int>(elapsed);
            int number = std::max(1, (remaining + 999) / 1000);
            RenderCountdown(renderer, renderConfig, number);
        } else if (state == AppState::Paused) {
            RenderFrame(renderer, game, pausedGameTimeMs, scrollSpeed, renderConfig, false);
            RenderPauseMenu(renderer, renderConfig, pauseMenuIndex);
        } else {
            std::vector<std::string> labels;
            labels.reserve(chartEntries.size());
            for (const auto& entry : chartEntries) {
                labels.push_back(entry.label);
            }
            RenderMenu(renderer, renderConfig, labels, selectedIndex);
        }

        SDL_RenderPresent(renderer);

        const GameStats& stats = game.GetStats();
        char title[256];
        if (state == AppState::Menu) {
            std::snprintf(title, sizeof(title), "SimpleMania | Select Beatmap");
        } else if (state == AppState::Ready) {
            std::snprintf(title, sizeof(title), "SimpleMania | Click Play or Press Space");
        } else if (state == AppState::Paused) {
            std::snprintf(title, sizeof(title), "SimpleMania | Paused");
        } else {
            std::snprintf(title, sizeof(title), "SimpleMania | Score %d | Combo %d | Speed %.2f",
                          game.GetTotalScore(), stats.combo, scrollSpeed);
        }
        SDL_SetWindowTitle(window, title);

        Uint32 frameElapsed = SDL_GetTicks() - frameStart;
        if (frameElapsed < static_cast<Uint32>(targetFrameMs)) {
            SDL_Delay(targetFrameMs - frameElapsed);
        }

        std::copy(keys, keys + SDL_NUM_SCANCODES, prevKeys.begin());
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
