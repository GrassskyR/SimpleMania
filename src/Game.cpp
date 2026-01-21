#include "Game.h"

#include <algorithm>

void Game::LoadChart(const Chart& chart) {
    // 复制谱面数据并建立轨道索引
    notes_ = chart.notes;
    keyCount_ = std::max(1, chart.keyCount);
    stats_ = GameStats();
    stats_.totalNotes = static_cast<int>(notes_.size());

    std::sort(notes_.begin(), notes_.end(), [](const Note& a, const Note& b) {
        return a.timeMs < b.timeMs;
    });

    laneIndices_.assign(keyCount_, {});
    for (size_t i = 0; i < notes_.size(); ++i) {
        int lane = notes_[i].lane;
        if (lane < 0) {
            lane = 0;
        } else if (lane >= keyCount_) {
            lane = keyCount_ - 1;
        }
        notes_[i].lane = lane;
        laneIndices_[lane].push_back(static_cast<int>(i));
    }

    laneCursor_.assign(keyCount_, 0);
}

void Game::Update(int nowMs) {
    // 超过Good窗口未击中则判Miss
    for (int lane = 0; lane < keyCount_; ++lane) {
        auto& cursor = laneCursor_[lane];
        auto& indices = laneIndices_[lane];
        while (cursor < indices.size()) {
            int idx = indices[cursor];
            Note& note = notes_[idx];
            if (note.judged) {
                ++cursor;
                continue;
            }
            if (nowMs - note.timeMs > goodWindowMs_) {
                ApplyJudge(note, JudgeGrade::Miss, nowMs);
                ++cursor;
                continue;
            }
            break;
        }
    }
}

JudgeGrade Game::HandleInput(int lane, int nowMs) {
    // 在对应轨道寻找最近未判定音符
    if (lane < 0 || lane >= keyCount_) {
        return JudgeGrade::None;
    }

    auto& cursor = laneCursor_[lane];
    auto& indices = laneIndices_[lane];
    while (cursor < indices.size()) {
        int idx = indices[cursor];
        Note& note = notes_[idx];
        if (note.judged) {
            ++cursor;
            continue;
        }

        int delta = nowMs - note.timeMs;
        int absDelta = delta < 0 ? -delta : delta;
        if (delta < -goodWindowMs_) {
            return JudgeGrade::None;
        }
        if (absDelta <= perfectWindowMs_) {
            ApplyJudge(note, JudgeGrade::Perfect, nowMs);
            ++cursor;
            return JudgeGrade::Perfect;
        }
        if (absDelta <= goodWindowMs_) {
            ApplyJudge(note, JudgeGrade::Good, nowMs);
            ++cursor;
            return JudgeGrade::Good;
        }
        if (delta > goodWindowMs_) {
            ApplyJudge(note, JudgeGrade::Miss, nowMs);
            ++cursor;
            return JudgeGrade::Miss;
        }
        return JudgeGrade::None;
    }
    return JudgeGrade::None;
}

void Game::ApplyJudge(Note& note, JudgeGrade grade, int nowMs) {
    // 记录判定、连击与计分
    if (note.judged) {
        return;
    }
    note.judged = true;
    stats_.judgedNotes += 1;
    stats_.lastJudge = grade;
    stats_.lastJudgeTimeMs = nowMs;
    switch (grade) {
        case JudgeGrade::Perfect:
            stats_.judgementPoints += 100;
            stats_.combo += 1;
            stats_.perfectCount += 1;
            break;
        case JudgeGrade::Good:
            stats_.judgementPoints += 65;
            stats_.combo += 1;
            stats_.goodCount += 1;
            break;
        case JudgeGrade::Miss:
            stats_.combo = 0;
            stats_.missCount += 1;
            break;
        case JudgeGrade::None:
            break;
    }
    if (stats_.combo > stats_.maxCombo) {
        stats_.maxCombo = stats_.combo;
    }
}

int Game::GetJudgementScore() const {
    // 判定分：Perfect=100, Good=65，满分900000
    if (stats_.totalNotes <= 0) {
        return 0;
    }
    double ratio = static_cast<double>(stats_.judgementPoints) /
                   (static_cast<double>(stats_.totalNotes) * 100.0);
    return static_cast<int>(ratio * 900000.0 + 0.5);
}

int Game::GetComboScore() const {
    // 连击分：maxCombo / totalNotes * 100000
    if (stats_.totalNotes <= 0) {
        return 0;
    }
    double ratio = static_cast<double>(stats_.maxCombo) /
                   static_cast<double>(stats_.totalNotes);
    return static_cast<int>(ratio * 100000.0 + 0.5);
}

int Game::GetTotalScore() const {
    return GetJudgementScore() + GetComboScore();
}

double Game::GetAccuracy() const {
    // Accuracy只按已判定音符计算
    if (stats_.judgedNotes <= 0) {
        return 100.0;
    }
    double ratio = static_cast<double>(stats_.judgementPoints) /
                   (static_cast<double>(stats_.judgedNotes) * 100.0);
    return ratio * 100.0;
}
