#pragma once

#include <string>
#include <vector>

struct TimingPoint {
    // 时间点与节拍信息（毫秒与拍长）
    double timeMs = 0.0;
    double beatLengthMs = 0.0;
    int meter = 4;
    bool inherited = false;
};

struct Note {
    // 单个下落音符（lane为轨道编号）
    int lane = 0;
    int timeMs = 0;
    int endTimeMs = 0;
    bool isHold = false;
    bool judged = false;
};

struct Chart {
    // 谱面基础信息与所有音符
    std::string title;
    std::string artist;
    std::string version;
    std::string audioFilename;
    int keyCount = 4;
    double baseBpm = 120.0;
    std::vector<TimingPoint> timingPoints;
    std::vector<Note> notes;
};
