#pragma once

#include <string>
#include <vector>

#include "Chart.h"

enum class JudgeGrade {
    None,
    Perfect,
    Good,
    Miss
};

struct GameStats {
    int combo = 0;
    int maxCombo = 0;
    int perfectCount = 0;
    int goodCount = 0;
    int missCount = 0;
    int totalNotes = 0;
    int judgedNotes = 0;
    int judgementPoints = 0;
    JudgeGrade lastJudge = JudgeGrade::None;
    int lastJudgeTimeMs = -999999;
};

class Game {
public:
    void LoadChart(const Chart& chart);
    void Update(int nowMs);
    JudgeGrade HandleInput(int lane, int nowMs);

    const std::vector<Note>& GetNotes() const { return notes_; }
    int GetKeyCount() const { return keyCount_; }
    int GetPerfectWindow() const { return perfectWindowMs_; }
    int GetGoodWindow() const { return goodWindowMs_; }
    const GameStats& GetStats() const { return stats_; }
    int GetTotalNotes() const { return stats_.totalNotes; }
    int GetJudgementScore() const;
    int GetComboScore() const;
    int GetTotalScore() const;
    double GetAccuracy() const;
    JudgeGrade GetLastJudge() const { return stats_.lastJudge; }
    int GetLastJudgeTimeMs() const { return stats_.lastJudgeTimeMs; }

private:
    void ApplyJudge(Note& note, JudgeGrade grade, int nowMs);

    std::vector<Note> notes_;
    std::vector<std::vector<int>> laneIndices_;
    std::vector<size_t> laneCursor_;
    int keyCount_ = 4;
    int perfectWindowMs_ = 80;
    int goodWindowMs_ = 160;
    GameStats stats_;
};
