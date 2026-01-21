#pragma once

#include <string>
#include <vector>

struct TimingPoint {
    double timeMs = 0.0;
    double beatLengthMs = 0.0;
    int meter = 4;
    bool inherited = false;
};

struct Note {
    int lane = 0;
    int timeMs = 0;
    int endTimeMs = 0;
    bool isHold = false;
    bool judged = false;
};

struct Chart {
    std::string title;
    std::string artist;
    std::string audioFilename;
    int keyCount = 4;
    double baseBpm = 120.0;
    std::vector<TimingPoint> timingPoints;
    std::vector<Note> notes;
};
